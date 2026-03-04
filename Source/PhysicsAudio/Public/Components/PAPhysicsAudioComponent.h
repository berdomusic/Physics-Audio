// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AkComponent.h"
#include "AkAudioEvent.h"
#include "System/PhysicsAudioStructs.h"
#include "PAPhysicsAudioComponent.generated.h"

/**
 * Physics Audio Component that handles impact, slide, and roll detection
 * with proper mass-based normalization and RTPC ranges
 */
UCLASS()
class PHYSICSAUDIO_API UPAPhysicsAudioComponent : public UAkComponent, public IProjectileInterface
{
	GENERATED_BODY()
	
	UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer);
	
public:
	void OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent, const FPAPhysicsActorAudioHandle& InAudioProperties, float InMassOverride);
	void OnDetachedFromPhysicsComponent();	
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	FORCEINLINE void SetSquaredDistanceToClosestListener(float InDistance) { DistanceToClosestListenerSquared = InDistance; }
protected:	
	UFUNCTION()
	virtual void OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
	                                              const FVector& InProjectileImpulse) override;	
	// Mass and physics properties
	void SetMassData();
	float ObjectMass;
	float ObjectMassSqrt;
	float ObjectMassOverride;
	
	// Detection thresholds (mass-normalized)
	float ImpactThreshold;
	float RollThreshold;
	
	// State tracking
	bool bIsSliding;
	bool bIsRolling;
	float TimeSinceLastRollDetection;
	
	// Velocity tracking
	float PreviousVelocityMagnitude;
	float CurrentVelocityMagnitude;
	
	// Rolling specific
	float AngularSpeed;
	float CurrentAngularSpeed;
	float PreviousAngularSpeed;
	
	// Audio properties
	FPAPhysicsActorAudioHandle PhysicsActorAudioProperties;
	UPROPERTY()
	UAkAudioEvent* ImpactSound;
	UPROPERTY()
	UAkAudioEvent* SlideSound;
	UPROPERTY()
	UAkAudioEvent* RollSound;
	UPROPERTY()
	UAkAudioEvent* ProjectileSound;
	UPROPERTY()
	UAkAudioEvent* DestructionSound;
	
	void LoadAkAudioEvents();
	void OnAkAudioEventsLoaded();
	bool bAkAudioEventsReady;
	
	// RTPC assets
	UPROPERTY()
	UPAPhysicsRTPCs* RTPCAssets;
	float CurrentSlideRTPCValue;
	float CurrentRollRTPCValue;
	
	// Audibility
	float DistanceToClosestListenerSquared;
	bool IsAkEventValidAndAudible(const UAkAudioEvent* InEvent) const;
	bool bSlideAudible;
	bool bRollAudible;
	
	// Component references
	UPROPERTY()
	UPrimitiveComponent* ParentComponent;
	
	// Cooldowns
	float ImpactCooldown;
	float SlideCooldown;
	float RollCooldown;
	
	// Helper functions
	void UpdatePhysicsState(float DeltaTime);
	bool bPhysicsStateUpdated;
	void UpdateRTPCValues();
	static float NormalizeByMass(float InValue, float InMass, float InExponent = .5f);
	bool ShouldPlayImpact(float InImpulseMagnitude) const;
	bool bGrounded = true;
	const float GroundedThreshold = 50.f;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};