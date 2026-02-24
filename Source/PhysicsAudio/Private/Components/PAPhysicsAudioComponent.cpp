// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PAPhysicsAudioComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

UPAPhysicsAudioComponent::UPAPhysicsAudioComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPAPhysicsAudioComponent::OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent,
                                                            const FPAPhysicsActorAudioHandle& InAudioProperties)
{
	check(InPhysicsComponent);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	PhysicsActorAudioProperties = InAudioProperties;
	ParentComponent = InPhysicsComponent;
	ParentComponent->OnComponentHit.AddUniqueDynamic(this, & UPAPhysicsAudioComponent::OnComponentHit);
}

void UPAPhysicsAudioComponent::OnDetachedFromPhysicsComponent()
{
	if (IsValid(ParentComponent))	
		ParentComponent->OnComponentHit.RemoveAll(this);	
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UPAPhysicsAudioComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	CurrentVelocity = ParentComponent->GetPhysicsLinearVelocity().Size();
	if(CheckVelocityDelta())
	{
		LastPlayedVelocity = CurrentVelocity;
		UKismetSystemLibrary::PrintString(GetWorld(), "Bump", true, false);
		//play sound logic
	}
}

bool UPAPhysicsAudioComponent::CheckVelocityDelta() const
{
	const bool bComparedVelocities = UKismetMathLibrary::Abs(CurrentVelocity - LastPlayedVelocity) > PhysicsActorAudioProperties.MinVelocityDeltaToSpawnSound;
	return CurrentVelocity > PhysicsActorAudioProperties.MinVelocityToSpawnCollisionSound && bComparedVelocities;
}

void UPAPhysicsAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ParentComponent))	
		ParentComponent->OnComponentHit.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}
