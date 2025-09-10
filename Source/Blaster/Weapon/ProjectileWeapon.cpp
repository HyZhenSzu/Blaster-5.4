// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
    Super::Fire(HitTarget);

    if (!HasAuthority()) return;
    APawn* InstigatorPawn = Cast<APawn>(GetOwner());
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    if (MuzzleFlashSocket)
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

        FVector StartLocation = GetWeaponMesh()->GetSocketLocation("MuzzleFlash");
        FVector ToTarget = HitTarget - StartLocation;

        /*FVector ToTarget = HitTarget - SocketTransform.GetLocation();*/

        // 在射击函数中添加调试绘制
        /*FVector StartLocation = SocketTransform.GetLocation();*/

        //    // 可视化调试（显示2秒）
        //DrawDebugSphere(
        //    GetWorld(),
        //    StartLocation,
        //    10.0f,  // 半径
        //    12,     // 分段数
        //    FColor::Red,
        //    false,
        //    2.0f    // 持续时间
        //);

        //FVector WeaponLocation = GetWeaponMesh()->GetComponentLocation();
        //DrawDebugSphere(GetWorld(), WeaponLocation, 5.0f, 12, FColor::Blue); // 武器根位置(蓝)


        FRotator TargetRotation = ToTarget.Rotation();
        if (ProjectileClass && InstigatorPawn)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner();
            SpawnParams.Instigator = InstigatorPawn;
            UWorld* World = GetWorld();
            if (World)
            {
                //World->SpawnActor<AProjectile>(
                //    ProjectileClass,
                //    SocketTransform.GetLocation(),
                //    TargetRotation,
                //    SpawnParams
                //);

                World->SpawnActor<AProjectile>(
                    ProjectileClass,
                    StartLocation,
                    TargetRotation,
                    SpawnParams
                );
            }
        }
    }
}
