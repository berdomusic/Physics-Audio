// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "System/PhysicsAudioStructs.h"
#include "PAPhysicsActor.generated.h"

UCLASS()
class PHYSICSAUDIO_API APAPhysicsActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APAPhysicsActor();
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	USphereComponent* SphereCollision;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	UStaticMesh* StaticMesh;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	UStaticMeshComponent* StaticMeshComponent;
	
	UPROPERTY(BlueprintReadWrite)
	FPAPhysicsActorAudioHandle PhysicsAudioProperties;
	UPROPERTY(BlueprintReadWrite)
	bool bAllowPhysicsSounds;
	
protected:
	
	virtual void OnConstruction(const FTransform& Transform) override;
	UFUNCTION()
	void OnPhysicsComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION(BlueprintCallable)
	void Init();
	virtual void BeginPlay() override;	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	FTimerHandle InitTimerHandle;
	FTimerHandle DeactivationTimerHandle;
	UPROPERTY(BlueprintReadOnly)
	TArray<AActor*> OverlappedActors;
	
	UFUNCTION()
	void OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void RetriggerDeactivationTimer();
	UFUNCTION()
	void OnActivationEndOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	bool ShouldDeactivatePhysicsAudio() const;
	void DeactivatePhysicsAudio();
};
