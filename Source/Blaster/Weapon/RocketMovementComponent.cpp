// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketMovementComponent.h"

/*
*  HandleBlockingHit deal with simulate after collision, if simulated, call HandleImpact.
*/
URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
    Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
    // EHandleBlockingHitResult::AdvanceNextSubstep means advance to next simulate,
    // it always used to when blocking by an object but the projectile still needs to move.
    return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
    // Rocket should not stop; only explode when their CollisionBox detects to hit.
}
