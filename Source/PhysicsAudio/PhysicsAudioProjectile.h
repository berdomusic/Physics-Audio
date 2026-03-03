// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PAPhysicsAudioComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/MapErrors.h"
#include "PhysicsAudioProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS(config=Game)
class APhysicsAudioProjectile : public AActor
{
	GENERATED_BODY()

	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PhysicsAudio")
	UPAPhysicsAudioComponent* AudioComponent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PhysicsAudio")
	FDataTableRowHandle PhysicsAudioHandle;
	UPROPERTY(BlueprintReadOnly, Category = "PhysicsAudio")
	FPAPhysicsActorAudioHandle ProjectileAudioProperties;
	UPROPERTY()
	UStaticMeshComponent* StaticMeshComponent;
	
	APhysicsAudioProjectile();

	/** called when projectile hits something */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }
	/** Returns StaticMeshComponent subobject **/
	UStaticMeshComponent* GetStaticMeshComponent() const;
protected:
	virtual void BeginPlay() override;
};

