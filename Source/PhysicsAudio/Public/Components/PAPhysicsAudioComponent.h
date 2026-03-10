// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AkComponent.h"
#include "AkAudioEvent.h"
#include "System/PhysicsAudioSettings.h"
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
	void TryReturnComponentToPool();

public:
	void OnAttachedToSimulatingComponent(UPrimitiveComponent* InComponent,
	                                     const FPAPhysicsActorAudioProperties& InAudioProperties);
	void OnAttachedToNonSimulatingComponent(UPrimitiveComponent* InComponent,
	                                        const FPAPhysicsActorAudioProperties& InAudioProperties);
	void OnDetachedFromPhysicsComponent();
	void OnParentDestroyed();
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	FORCEINLINE void SetSquaredDistanceToClosestListener(float InDistance)
	{
		DistanceToClosestListenerSquared = InDistance;
	}
protected:
	UFUNCTION()
	virtual void OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
	                                              const FVector& InProjectileImpulse) override;	
	void OnAttachedToComponent_Internal(UPrimitiveComponent* InComponent, 
		const FPAPhysicsActorAudioProperties& InAudioProperties);	
	
	// Mass and physics properties
	void SetMassData();
	float ObjectMass;
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
	FPAPhysicsActorAudioProperties PhysicsActorAudioProperties;
	UPROPERTY()
	TArray<FPAPhysicsAudioEvent> AudioEventsSoftRefs;
	UPROPERTY()
	TArray<UAkAudioEvent*> AudioEvents;
	
	FORCEINLINE UAkAudioEvent* GetAkEventByType(EPAEventType InType)
	{
		const int32 idx = static_cast<int32>(InType);
		return AudioEvents.IsValidIndex(idx) ? AudioEvents[idx] : nullptr;
	}
	void LoadAkAudioEvents();
	void OnAkAudioEventsLoaded();
	
	// RTPC assets
	UPROPERTY()
	UPAPhysicsRTPCs* RTPCAssets;
	float PreviousImpactRTPCValue;
	float PreviousSlideRTPCValue;
	float PreviousRollRTPCValue;
	
	// Audibility
	float DistanceToClosestListenerSquared;
	bool IsAkEventValidAndAudible(const UAkAudioEvent* InEvent) const;
	bool bSlideAudible;
	bool bRollAudible;
	
	// Component references
	UPROPERTY()
	UPrimitiveComponent* ParentComponent;
	
	// Cooldowns
	UPROPERTY(BlueprintReadOnly)
	float CurrentImpactCooldown = .1f;
	float ImpactCooldownThreshold = .1f;	
	float CurrentSlideCooldown = .1f;
	float SlideCooldownThreshold = .1f;	
	float CurrenRollCooldown = .1f;
	float RollCooldownThreshold = .1f;
	void SetCooldownVariables();
	
	// Helper functions
	void UpdatePhysicsState(float DeltaTime);
	bool bPhysicsStateUpdated;
	void UpdateRTPCValues();
	static float NormalizeByMass(float InValue, float InMass, float InExponent = .5f);
	bool ShouldPlayImpact(float InImpulseMagnitude) const;
	bool bGrounded;
	const float GroundedThreshold = 50.f;
	
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
};