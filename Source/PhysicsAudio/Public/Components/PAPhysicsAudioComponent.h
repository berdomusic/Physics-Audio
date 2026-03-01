// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AkComponent.h"
#include "AkAudioEvent.h"
#include "System/PhysicsAudioStructs.h"

#include "PAPhysicsAudioComponent.generated.h"

/**
 * 
 */
UCLASS()
class PHYSICSAUDIO_API UPAPhysicsAudioComponent : public UAkComponent, public IProjectileInterface
{
	GENERATED_BODY()
	
	UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer);
	
public:	
	void OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent, const FPAPhysicsActorAudioHandle& InAudioProperties, float InMassOverride);
	void OnDetachedFromPhysicsComponent();
	void SetVelocityRTPC(bool bInAccountForDelta) const;

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
protected:
	UFUNCTION()
	virtual void OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
	                                              const FVector& InProjectileImpulse) override; 
	void SetMassData();
	float ObjectMass;
	float ObjectMassOverride;
	float SlideThreshold;
	float MinVelocityDeltaToSpawnImpactSound;
	
	UPROPERTY()
	UAkRtpc* VelocityRTPC;
	
	FPAPhysicsActorAudioHandle PhysicsActorAudioProperties;
	
	UPROPERTY()
	UAkAudioEvent* ImpactSound;
	UPROPERTY()
	UAkAudioEvent* SlideSound;
	UPROPERTY()
	UAkAudioEvent* ProjectileSound;
	UPROPERTY()
	UAkAudioEvent* DestructionSound;
	void LoadAkAudioEvents();
	void OnAkAudioEventsLoaded();
	bool bAkAudioEventsLoaded;
	
	float AudibilityRangeSquared = 0.f;
	void SetAudibilityRange();
	bool IsAudible() const;
	
	UPROPERTY()
	UPrimitiveComponent* ParentComponent;
	
	bool bIsSliding;
	float CurrentVelocity;
	float PreviousVelocity;
	float CurrentVelocityDelta;
	bool CheckVelocityDelta();
	
	float ImpactCooldown;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
