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
class PHYSICSAUDIO_API UPAPhysicsAudioComponent : public UAkComponent, public IPhysicsAudioInterface
{
	GENERATED_BODY()
	
	UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer);
	
public:	
	void OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent, const FPAPhysicsActorAudioHandle& InAudioProperties);
	void OnDetachedFromPhysicsComponent();
	void SetVelocityRTPC() const;

protected:
	UFUNCTION()
	virtual void OnHitByProjectile_Implementation(AActor* HitActor, UPrimitiveComponent* HitComp, AActor* ProjectileActor, UPrimitiveComponent* ProjectileComp, FVector NormalImpulse, const FHitResult& Hit) override;
	void SetMassData();
	float ObjectMass;
	float SlideThreshold;
	float MinVelocityToSpawnImpactSound;
	float MinVelocityDeltaToSpawnImpactSound;
	
	UPROPERTY()
	UAkRtpc* VelocityRTPC;
	
	FPAPhysicsActorAudioHandle PhysicsActorAudioProperties;
	
	UPROPERTY()
	UAkAudioEvent* ImpactSound;
	UPROPERTY()
	UAkAudioEvent* SlideSound;
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
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	float CurrentVelocity;
	float LastPlayedVelocity;	
	bool CheckVelocityDelta() const;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
