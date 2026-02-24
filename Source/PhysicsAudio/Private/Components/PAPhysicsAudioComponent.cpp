// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PAPhysicsAudioComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "System/PAFunctionLibrary.h"

UPAPhysicsAudioComponent::UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPAPhysicsAudioComponent::OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent,
                                                            const FPAPhysicsActorAudioHandle& InAudioProperties)
{
	check(InPhysicsComponent);
	VelocityRTPC = InAudioProperties.RTPC_Handle->VelocityRTPC;
	check(IsValid(VelocityRTPC));
	PrimaryComponentTick.SetTickFunctionEnable(true);
	PhysicsActorAudioProperties = InAudioProperties;
	ParentComponent = InPhysicsComponent;
	ParentComponent->OnComponentHit.AddUniqueDynamic(this, & UPAPhysicsAudioComponent::OnComponentHit);
	
	SetMassData();
	LoadAkAudioEvents();
}

void UPAPhysicsAudioComponent::OnDetachedFromPhysicsComponent()
{
	if (IsValid(ParentComponent))	
		ParentComponent->OnComponentHit.RemoveAll(this);	
	PrimaryComponentTick.SetTickFunctionEnable(false);
	
	bAkAudioEventsLoaded = false;
	CollisionSound = nullptr;
	DestructionSound = nullptr;
}

void UPAPhysicsAudioComponent::PlayCollisionSound()
{
	const float mappedVelocity = UKismetMathLibrary::MapRangeClamped(CurrentVelocity, 50.f, 500.f, .1f, 1.f);
	SetRTPCValue(VelocityRTPC, mappedVelocity, 0, FString());
	PostAkEvent(CollisionSound, 0, FOnAkPostEventCallback());
}

void UPAPhysicsAudioComponent::SetMassData()
{
	ObjectMass = ParentComponent->GetMass();
	
	UAkRtpc* massRTPC = PhysicsActorAudioProperties.RTPC_Handle->MassRTPC;
	ensure(massRTPC);
	float massRTPCValue = UKismetMathLibrary::MapRangeClamped(ObjectMass, .1f, 250.f, 0.f, 2.f);
	SetRTPCValue(massRTPC, massRTPCValue, 0, FString());
	
	MinVelocityToSpawnCollisionSound = UKismetMathLibrary::MapRangeClamped(ObjectMass, 20.f, 1200, 100.f, 10.f);
	MinVelocityDeltaToSpawnSound = UKismetMathLibrary::MapRangeClamped(ObjectMass, 20.f, 1200.f, 50.f, 5.f);
}

void UPAPhysicsAudioComponent::LoadAkAudioEvents()
{
	TArray<FSoftObjectPath> AssetsToLoad;

	if (!PhysicsActorAudioProperties.CollisionSound.IsNull())
		AssetsToLoad.Add(PhysicsActorAudioProperties.CollisionSound.ToSoftObjectPath());

	if (!PhysicsActorAudioProperties.DestructionSound.IsNull())
		AssetsToLoad.Add(PhysicsActorAudioProperties.DestructionSound.ToSoftObjectPath());

	if (AssetsToLoad.IsEmpty())
		return;	

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(
			this,
			&UPAPhysicsAudioComponent::OnAkAudioEventsLoaded
		)
	);
}

void UPAPhysicsAudioComponent::OnAkAudioEventsLoaded()
{
	CollisionSound = PhysicsActorAudioProperties.CollisionSound.Get();
	DestructionSound = PhysicsActorAudioProperties.DestructionSound.Get();
	bAkAudioEventsLoaded = true;
}

void UPAPhysicsAudioComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	CurrentVelocity = ParentComponent->GetPhysicsLinearVelocity().Size();
	if(CheckVelocityDelta())
	{
		LastPlayedVelocity = CurrentVelocity;
		UKismetSystemLibrary::PrintString(GetWorld(), "Bump", true, false);
		if (bAkAudioEventsLoaded)
		{
			if (IsValid(Hit.PhysMaterial.Get()))
				SetSwitch(UPAFunctionLibrary::GetAkSwitchFromSurface(Hit.PhysMaterial->SurfaceType));			
			PlayCollisionSound();
		}		
	}
}

bool UPAPhysicsAudioComponent::CheckVelocityDelta() const
{
	const bool bComparedVelocities = UKismetMathLibrary::Abs(CurrentVelocity - LastPlayedVelocity) > MinVelocityDeltaToSpawnSound;
	return CurrentVelocity > MinVelocityToSpawnCollisionSound && bComparedVelocities;
}

void UPAPhysicsAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ParentComponent))	
		ParentComponent->OnComponentHit.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}
