// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PAHealthComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "System/PhysicsAudioStructs.h"
#include "PAPhysicsActor.generated.h"

UCLASS()
class PHYSICSAUDIO_API APAPhysicsActor : public AActor, public IProjectileInterface, public IDamageInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APAPhysicsActor();
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	USphereComponent* SphereCollision;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	UStaticMeshComponent* StaticMeshComponent;
	UPROPERTY()
	UPAHealthComponent* HealthComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FDataTableRowHandle PhysicsAudioHandle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FDataTableRowHandle DestructionAudioHandle;
	UPROPERTY(BlueprintReadOnly)
	FPAPhysicsActorAudioHandle PhysicsAudioProperties;
	UPROPERTY(BlueprintReadOnly)
	FPAPhysicsActorAudioHandle DestructionAudioProperties;	
	
	UPROPERTY(BlueprintReadWrite)
	TArray<UStaticMeshComponent*> DestructionMeshes;
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void TEST_Destroy();
	
protected:
	
	bool bAllowPhysicsSounds;
	bool bAllowDestructionSounds;
	
	virtual void OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
	                                              const FVector& InProjectileImpulse) override;
	virtual void OnDamageDealt_Implementation(AActor* Dealer, const FHitResult& Hit, const FVector& InImpulse) override;
	UFUNCTION()
	void OnPhysicsComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION(BlueprintCallable)
	void Init();
	void GetDestructibleMeshes();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	FTimerHandle InitTimerHandle;
	FTimerHandle DeactivationTimerHandle;
	UPROPERTY(BlueprintReadOnly)
	TArray<AActor*> OverlappedActors;
	
	UFUNCTION()
	void OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnActivationEndOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UFUNCTION()
	void OnDeath(AActor* Dealer, const FHitResult& Hit, const FVector& Impulse);

	static bool ShouldActivatePhysicsAudio(const AActor* OtherActor, UPrimitiveComponent* OtherComp);
	bool ShouldDeactivatePhysicsAudio() const;
	void DeactivatePhysicsAudio();
	void RetriggerDeactivationTimer();
};
