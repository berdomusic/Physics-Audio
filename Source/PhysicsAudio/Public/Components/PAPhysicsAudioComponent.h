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
	float SlideThreshold;
	float RollThreshold;
	float VelocityDeltaThreshold;
	
	// State tracking
	bool bIsSliding;
	bool bIsRolling;
	bool bWasRolling;
	float TimeSinceLastRollDetection;
	
	// Velocity tracking
	FVector PreviousVelocity;
	FVector CurrentVelocity;
	float PreviousVelocityMagnitude;
	float CurrentVelocityMagnitude;
	float VelocityDeltaMagnitude;
	
	// Rolling specific
	FVector PreviousAngularVelocity;
	FVector CurrentAngularVelocity;
	float AngularSpeed;
	float CurrentAngularSpeed;
	float PreviousAngularSpeed;
	
	// Audio events
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
	float AudibilityRangeSquared = 0.f;
	void SetAudibilityRange();
	bool bIsAudible;
	bool IsAudible() const;
	
	// Component references
	UPROPERTY()
	UPrimitiveComponent* ParentComponent;
	FPAPhysicsActorAudioHandle PhysicsActorAudioProperties;
	
	// Cooldowns
	float ImpactCooldown;
	float SlideCooldown;
	float RollCooldown;
	
	// Helper functions
	void UpdatePhysicsState(float DeltaTime);
	void CalculateRollingState(float DeltaTime);
	void UpdateRTPCValues();
	static float NormalizeByMass(float InValue, float InMass, float InExponent = .5f);
	bool ShouldPlayImpact(float InImpulseMagnitude, float InVelocityMagnitude, float InVelocityDelta) const;
	bool IsOnGround() const;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};