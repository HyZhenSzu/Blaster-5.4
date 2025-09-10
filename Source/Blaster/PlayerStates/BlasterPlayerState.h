// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

    /*
    *  Replication Notifies.
    */
    UFUNCTION()
	virtual void OnRep_Defeats();
	virtual void OnRep_Score() override;

    void AddToDefeats(int32 AmountDefeats);
	void AddToScore(float AmountScore);

private:
    UPROPERTY()
	class ABlasterCharacter* Character;
    UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};
