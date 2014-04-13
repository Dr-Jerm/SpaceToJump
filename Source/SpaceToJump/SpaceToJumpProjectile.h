// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SpaceToJumpProjectile.generated.h"

UCLASS(config=Game)
class ASpaceToJumpProjectile : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	TSubobjectPtr<USphereComponent> CollisionComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement)
	TSubobjectPtr<class UProjectileMovementComponent> ProjectileMovement;

	/** called when projectile hits something */
	UFUNCTION()
	void OnOverlap(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};

