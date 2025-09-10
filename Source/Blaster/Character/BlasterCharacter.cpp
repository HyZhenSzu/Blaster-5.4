// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Blaster.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Blaster/PlayerStates/BlasterPlayerState.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    //// Don't rotate when the controller rotates. Let that just affect the camera.
    //bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    //bUseControllerRotationRoll = false;

    //// Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
    //GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

    //// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    //// instead of recompiling to adjust them
    //GetCharacterMovement()->JumpZVelocity = 700.f;
    //GetCharacterMovement()->AirControl = 0.35f;
    //GetCharacterMovement()->MaxWalkSpeed = 500.f;
    //GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    //GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    //GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
    
    MoveAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Move.IA_Move")));
    LookAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Look.IA_Look")));
    JumpAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Jump.IA_Jump")));
    EquipAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Equip.IA_Equip")));
    CrouchAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Crouch.IA_Crouch")));
    AimingAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Aiming.IA_Aiming")));
    FireAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Fire.IA_Fire")));
    ReloadAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/Inputs/IA_Reload.IA_Reload")));

    ImcBlasterMappingContext = Cast<UInputMappingContext>(StaticLoadObject(UInputMappingContext::StaticClass(), nullptr, TEXT("/Game/Blueprints/MappingContexts/IMC_BlasterMappingContext.IMC_BlasterMappingContext")));

    // Add and Set up Camera and SpringArm Components
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetMesh());
    CameraBoom->TargetArmLength = 600.f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
    OverheadWidget->SetupAttachment(RootComponent);

    Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
    Combat->SetIsReplicated(true);

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);

    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    NetUpdateFrequency = 66.f;
    MinNetUpdateFrequency = 33.f;

    DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
    Super::OnRep_ReplicatedMovement();
    SimProxiesTurn();
    TimeSinceLastMovementReplication = 0.f;
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

    if (IsLocallyControlled())
    {
        // Only local controlled character need to load the mapping context.
        SetMappingContextInput();
    }

    UpdateHealthHUD();

    if (HasAuthority())
    {
        OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
    }
}

void ABlasterCharacter::SetMappingContextInput()
{
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* EnhanceInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            if (ImcBlasterMappingContext)
            {
                EnhanceInputSubsystem->AddMappingContext(ImcBlasterMappingContext, 0);
            }
        }
    }
    else
        UE_LOG(LogTemp, Warning, TEXT("Player Controller failed to load..."));
}

void ABlasterCharacter::UpdateHealthHUD()
{
    BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
    if (BlasterPlayerController)
    {
        BlasterPlayerController->SetHUDHealthValue(Health, MaxHealth);
    }
}

void ABlasterCharacter::PollInit()
{
    if (BlasterPlayerState == nullptr)
    {
        BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
        if (BlasterPlayerState)
        {
            BlasterPlayerState->AddToScore(0.f);
            BlasterPlayerState->AddToDefeats(0);
        }
    }
}

void ABlasterCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RotateInPlace(DeltaTime);
    HideCharacterIfClose();
    PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
    if (bDisableGameplay)
    {
        bUseControllerRotationYaw = false;
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        return;
    }
    if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
    {
        AimOffSet(DeltaTime);
    }
    else
    {
        TimeSinceLastMovementReplication += DeltaTime;
        if (TimeSinceLastMovementReplication > 0.25f)
        {
            OnRep_ReplicatedMovement();
        }
        CalculateAO_Pitch();
    }
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);

        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABlasterCharacter::Jump);
        EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ABlasterCharacter::EquipButtonPressed);
        EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABlasterCharacter::CrouchButtonPressed);
        EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::AimingButtonPressed);
        EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Completed, this, &ABlasterCharacter::AimingButtonReleased);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::FireButtonPressed);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased);
        EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABlasterCharacter::ReloadButtonPressed);
    }
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
    DOREPLIFETIME(ABlasterCharacter, Health);
    DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    if (Combat)
    {
        Combat->Character = this;
    }
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && FireWeaponMontage)
    {
        AnimInstance->Montage_Play(FireWeaponMontage);
        FName SectionName;
        SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
        AnimInstance->Montage_JumpToSection(SectionName);
    }
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(-LookAxisVector.Y);
    }
}

void ABlasterCharacter::Jump()
{
    if (bDisableGameplay) return;
    if (bIsCrouched)
    {
        UnCrouch();
    }
    else
    {
        Super::Jump();
    }
}

void ABlasterCharacter::EquipButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    if (Value.Get<bool>())
    {
        if (Combat)
        {
            if (HasAuthority())
                Combat->EquipWeapon(OverlappingWeapon);
            else
                ServerEquipButtonPressed();
        }
    }
}

void ABlasterCharacter::CrouchButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    if (!bIsCrouched)
        Crouch();
    else
        UnCrouch();
}

void ABlasterCharacter::AimingButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    if (Combat)
    {
        Combat->SetAiming(true);
    }
}

void ABlasterCharacter::AimingButtonReleased(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    if (Combat)
    {
        Combat->SetAiming(false);
    }
}

float ABlasterCharacter::CalculateSpeed()
{
    FVector Velocity = GetVelocity();
    Velocity.Z = 0.f;
    return Velocity.Size();
}

void ABlasterCharacter::AimOffSet(float DeltaTime)
{
    if (Combat && Combat->EquippedWeapon == nullptr) return;
    float Speed = CalculateSpeed();
    bool bIsInAir = GetCharacterMovement()->IsFalling();
    if (Speed == 0.f && !bIsInAir) // standing still, not jumping
    {
        bRotateRootBone = true;
        FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
        AO_Yaw = DeltaAimRotation.Yaw;
        if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
        {
            InterpAO_Yaw = AO_Yaw;
        }
        bUseControllerRotationYaw = true;
        TurnInPlace(DeltaTime);
    }
    if (Speed > 0.f || bIsInAir) // running or jumping
    {
        bRotateRootBone = false;
        StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        AO_Yaw = 0.f;
        bUseControllerRotationYaw = true;
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    }

    CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
    AO_Pitch = GetBaseAimRotation().Pitch;
    if (AO_Pitch > 90.f && !IsLocallyControlled())
    {
        // map pitch from [270, 360) to [-90, 0)
        FVector2D InRange(270.f, 360.f);
        FVector2D OutRange(-90.f, 0.f);
        AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
    }
}

void ABlasterCharacter::SimProxiesTurn()
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

    bRotateRootBone = false;
    float Speed = CalculateSpeed();
    if (Speed > 0.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        return;
    }

    ProxyRotationLastFrame = ProxyRotation;
    ProxyRotation = GetActorRotation();
    ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;


    if (FMath::Abs(ProxyYaw) > TurnThresold)
    {
        // Turning Right
        if (ProxyYaw > TurnThresold)
        {
            TurningInPlace = ETurningInPlace::ETIP_Right;
        }
        else if (ProxyYaw < -TurnThresold)
        {
            TurningInPlace = ETurningInPlace::ETIP_Left;
        }
        else
        {
            TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        }
        return;
    }
    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::FireButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    if (Combat)
    {
        Combat->FireButtonPressed(true);
    }
}

void ABlasterCharacter::FireButtonReleased(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    if (Combat)
    {
        Combat->FireButtonPressed(false);
    }
}

void ABlasterCharacter::PlayHitMontage()
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && OnHitMontage)
    {
        AnimInstance->Montage_Play(OnHitMontage);
        FName SectionName;
        SectionName = FName("HitFront");
        AnimInstance->Montage_JumpToSection(SectionName);
    }
}

void ABlasterCharacter::ReloadButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay) return;
    if (Combat)
    {
        Combat->Reload();
    }
}


void ABlasterCharacter::PlayReloadMontage()
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && ReloadMontage)
    {
        AnimInstance->Montage_Play(ReloadMontage);
        FName SectionName;
        
        switch (Combat->EquippedWeapon->GetWeaponType())
        {
        case EWeaponType::EWT_AssaultRifle:
            SectionName = FName("Rifle");
            break;
        case EWeaponType::EWT_RocketLancher:
            SectionName = FName("Rifle");
            break;
        // Other WeaponType will be added here. 
        }
        AnimInstance->Montage_JumpToSection(SectionName);
    }
}


void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
    if (Combat)
    {
        Combat->EquipWeapon(OverlappingWeapon);
    }
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
    if (OverlappingWeapon)
    {
        OverlappingWeapon->ShowPickupWidget(true);
    }
    if (LastWeapon)
    {
        LastWeapon->ShowPickupWidget(false);
    }
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
    if (AO_Yaw > 90.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_Right;
    }
    else if(AO_Yaw < -90.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_Left;
    }
    if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
    {
        InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
        AO_Yaw = InterpAO_Yaw;
        if (FMath::Abs(AO_Yaw) < 20.f)
        {
            TurningInPlace = ETurningInPlace::ETIP_NotTurning;
            StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        }
    }
}

void ABlasterCharacter::HideCharacterIfClose()
{
    if (!IsLocallyControlled()) return;
    if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThresHold)
    {
        GetMesh()->SetVisibility(false);
        if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
        {
            Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
        }
    }
    else
    {
        GetMesh()->SetVisibility(true);
        if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
        {
            Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
        }
    }
}

void ABlasterCharacter::OnRep_Health()
{
    UpdateHealthHUD();
    PlayHitMontage();
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
    if (OverlappingWeapon)
    {
        OverlappingWeapon->ShowPickupWidget(false);
    }
    OverlappingWeapon = Weapon;
    if (IsLocallyControlled())
    {
        if (OverlappingWeapon)
        {
            OverlappingWeapon->ShowPickupWidget(true);
        }
    }
}

bool ABlasterCharacter::IsWeaponEquipped()
{
    return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
    return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
    if (Combat == nullptr) return nullptr;
    return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
    if (Combat == nullptr) return FVector();
    return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
    if (Combat == nullptr) return ECombatState::ESC_DefaultMax;
    return Combat->CombatState;
}

void ABlasterCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
    Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
    UpdateHealthHUD();
    PlayHitMontage();

    if (Health == 0.f)
    {
        ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
        if (BlasterGameMode)
        {
            BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
            ABlasterPlayerController* AttackerPlayerController = Cast<ABlasterPlayerController>(InstigatorController);
            BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerPlayerController);
        }
    }
}

void ABlasterCharacter::Elim()
{
    if (Combat && Combat->EquippedWeapon)
    {
        Combat->EquippedWeapon->Dropped();
    }
    MulticastElim();
    // Start Timer
    GetWorld()->GetTimerManager().SetTimer(
        ElimTimer,
        this,
        &ABlasterCharacter::ElimTimerFinish,
        ElimDelay
    );
}

void ABlasterCharacter::MulticastElim_Implementation()
{
    if (BlasterPlayerController)
    {
        BlasterPlayerController->SetHUDAmmoValue(0);
        BlasterPlayerController->SetHUDCarriedAmmo(0);
    }
    bElimmed = true;
    PlayElimMontage();

    // Start dissolve effect.
    if (DissolveMaterialInstance)
    {
        DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
        GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.55f);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
    }
    StartDissolve();

    // Disable Character Movement
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();
    bDisableGameplay = true;
    GetCharacterMovement()->DisableMovement();
    if (Combat)
    {
        Combat->FireButtonPressed(false);
    }

    // Disable the Capsule and Mesh
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::ElimTimerFinish()
{
    ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
    if (BlasterGameMode)
    {
        BlasterGameMode->RequestRespawn(this, Controller);
    }
}

void ABlasterCharacter::PlayElimMontage()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && ElimMontage)
    {
        AnimInstance->Montage_Play(ElimMontage);
    }
}


void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
    if (DynamicDissolveMaterialInstance)
    {
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
    }
}

void ABlasterCharacter::StartDissolve()
{
    DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
    if (DissolveCurve && DissolveTimeline)
    {
        DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
        DissolveTimeline->Play();
    }
}


void ABlasterCharacter::Destroyed()
{
    Super::Destroyed();

    ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
    if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
    {
        Combat->EquippedWeapon->Destroy();
    }
}
