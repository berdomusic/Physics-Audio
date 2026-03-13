// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PAHealthComponent.h"

#include "AudioMixerBlueprintLibrary.h"

// Sets default values for this component's properties
UPAHealthComponent::UPAHealthComponent()
{
}

void UPAHealthComponent::OnDamageDealt_Implementation(AActor* Dealer, const FHitResult& Hit, const FVector& InImpulse)
{
	IDamageInterface::OnDamageDealt_Implementation(Dealer, Hit, InImpulse);
	const float damage = FMath::GetMappedRangeValueClamped(
		FVector2D (0.f, 300000.f),
		FVector2D(0.f, 25.f),
		InImpulse.Size()
		);
	CurrentHealth -= damage;
	OnHealthUpdate.Broadcast(CurrentHealth, MaxHealth);
	
	if (CurrentHealth < 0.f)
		OnDeath.Broadcast(Dealer, Hit, InImpulse);
}



