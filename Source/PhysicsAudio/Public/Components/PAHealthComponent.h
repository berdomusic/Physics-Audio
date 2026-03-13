// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "System/PhysicsAudioStructs.h"
#include "PAHealthComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PHYSICSAUDIO_API UPAHealthComponent : public UActorComponent, public IDamageInterface
{
	GENERATED_BODY()
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPAHealthComponent_HealthUpdateDelegate, float, InNewHealth, float, InMaxHealth);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPAHealthComponent_DeathDelegate, AActor*, Dealer, const FHitResult&, Hit, const FVector&, Impulse);

public:	
	// Sets default values for this component's properties
	UPAHealthComponent();
	
	void SetCanBeDamaged(bool CanBeDamaged) { bCanBeDamaged = CanBeDamaged; }
	UFUNCTION(BlueprintPure)
	bool CanBeDamaged() const { return bCanBeDamaged; }
	UFUNCTION(BlueprintPure)
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure)
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintPure)
	bool IsAlive() const { return CurrentHealth > 0.f; }
	
	UPROPERTY(BlueprintAssignable)
	FPAHealthComponent_HealthUpdateDelegate OnHealthUpdate;
	FPAHealthComponent_DeathDelegate OnDeath;
	
protected:	
	float MaxHealth = 100.f;
	float CurrentHealth = 100.f;
	bool bCanBeDamaged;
	virtual void OnDamageDealt_Implementation(AActor* Dealer, const FHitResult& Hit, const FVector& InImpulse) override;
};
