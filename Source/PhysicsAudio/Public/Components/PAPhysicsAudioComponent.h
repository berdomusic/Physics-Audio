// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AkComponent.h"
#include "System/PhysicsAudioStructs.h"
#include "PAPhysicsAudioComponent.generated.h"

/**
 * 
 */
UCLASS()
class PHYSICSAUDIO_API UPAPhysicsAudioComponent : public UAkComponent
{
	GENERATED_BODY()
	
	UPAPhysicsAudioComponent();
	
public:	
	void OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent, const FPAPhysicsActorAudioHandle& InAudioProperties);
	void OnDetachedFromPhysicsComponent();
	
protected:
	FPAPhysicsActorAudioHandle PhysicsActorAudioProperties;
	
	UPROPERTY()
	UPrimitiveComponent* ParentComponent;
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	float CurrentVelocity;
	float LastPlayedVelocity;	
	bool CheckVelocityDelta() const;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
