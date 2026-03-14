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
class PHYSICSAUDIO_API UPAPhysicsAudioComponent : public UAkComponent, 
	public IProjectileInterface, public IPhysicsAudioInterface, public ILookAtInterface
{
	GENERATED_BODY()
	
	UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer);
	void TryReturnComponentToPool();

public:
	void OnAttachedToSimulatingComponent(UPrimitiveComponent* InComponent,
	                                     const FPAPhysicsActorAudioProperties& InAudioProperties);
	void OnAttachedToNonSimulatingComponent(UPrimitiveComponent* InComponent,
	                                        const FPAPhysicsActorAudioProperties& InAudioProperties);
	void DeactivatePhysicsAudioComponent();
	void OnParentDestroyed();
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	FORCEINLINE void SetSquaredDistanceToClosestListener(float InDistance)
	{
		DistanceToClosestListenerSquared = InDistance;
	}
protected:
	virtual void OnPickup_Implementation(AActor* InInstigator) override;
	virtual void OnDrop_Implementation(AActor* InInstigator) override;
	virtual void OnPhysicsActorHit_Implementation(FVector NormalImpulse, const FHitResult& Hit) override;
	UFUNCTION()
	virtual void OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
	                                              const FVector& InProjectileImpulse) override;	
	void OnAttachedToComponent_Internal(UPrimitiveComponent* InComponent, 
		const FPAPhysicsActorAudioProperties& InAudioProperties);	
	
	// Mass and physics properties
	void SetMassData();
	float ObjectMass;
	float ObjectMassOverride;
	
	// Detection thresholds
	float ImpactVelocityThreshold;
	float RollThreshold;
	
	// State tracking
	bool bIsSliding;
	bool bIsRolling;
	bool bSlideAudible;
	bool bRollAudible;
	bool bSlideLoopPosted;
	bool bRollLoopPosted;
	bool bSlideInfinite;
	bool bRollInfinite;
	
	float TimeSinceLastRollDetection;	
	void StopContinuousSound(TArray<EPAEventType> InEventTypes);
	
	// Velocity tracking
	float PreviousVelocityMagnitude;
	float CurrentVelocityMagnitude;
	
	// Rolling specific
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
	void SetInfiniteEventsVariables(TArray<EPAEventType> InEventTypes =
		{ EPAEventType::Slide, EPAEventType::Roll });
	
	// RTPC assets
	UPROPERTY()
	UPAPhysicsRTPCs* RTPCAssets;
	float PreviousImpactRTPCValue;
	float PreviousSlideRTPCValue;
	float PreviousRollRTPCValue;
	
	// Audibility
	float DistanceToClosestListenerSquared;
	bool IsAkEventValidAndAudible(const UAkAudioEvent* InEvent) const;
	
	// Component references
	UPROPERTY()
	UPrimitiveComponent* ParentComponent;
	
	// Cooldowns
	float CurrentImpactCooldown = .1f;
	float ImpactCooldownThreshold = .1f;	
	float CurrentSlideCooldown = .1f;
	float SlideCooldownThreshold = .1f;	
	float CurrenRollCooldown = .1f;
	float RollCooldownThreshold = .1f;
	void SetCooldownVariables();	

	// Helper functions
	void UpdatePhysicsState(float DeltaTime);
	void UpdateRTPCValues();
	void HandleStoppingLoops();
	void HandlePlayingContinuousSounds();
	void HandlePlayingSlide();
	void HandlePlayingRoll();	
	
	static float NormalizeByMass(float InValue, float InMass, float InExponent = .5f);
	bool ShouldPlayImpact(float InImpulseMagnitude) const;
	bool bGrounded;
	bool bPickedUp;
	const float GroundedThreshold = 50.f;
	
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
};