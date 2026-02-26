// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PAPhysicsAudioComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PAPhysicsAudioSubsystem.h"
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
	ImpactSound = nullptr;
	SlideSound = nullptr;
	DestructionSound = nullptr;
}

void UPAPhysicsAudioComponent::SetVelocityRTPC() const
{
	const float mappedVelocity = 
		FMath::GetMappedRangeValueClamped(
			FVector2D(50.f, 500.f),
			FVector2D(.1f,1.f),
			CurrentVelocity);
	SetRTPCValue(VelocityRTPC, mappedVelocity, 0, FString());
}

void UPAPhysicsAudioComponent::OnHitByProjectile_Implementation(AActor* HitActor, UPrimitiveComponent* HitComp,
	AActor* ProjectileActor, UPrimitiveComponent* ProjectileComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bAkAudioEventsLoaded || !IsAudible()) // check if we even bother playing sound
		return;
	if (ImpactSound == nullptr)
		return;
	UKismetSystemLibrary::PrintString(GetWorld(), "Projectile", true, false, FLinearColor::Red, .2f);
	CurrentVelocity = 500.f;
	SetVelocityRTPC();
	PostAkEvent(ImpactSound, 0, FOnAkPostEventCallback());
}

void UPAPhysicsAudioComponent::SetMassData()
{
	ObjectMass = ParentComponent->GetMass();
	
	UAkRtpc* massRTPC = PhysicsActorAudioProperties.RTPC_Handle->MassRTPC;
	ensure(massRTPC);
	const float massRTPCValue = 
		FMath::GetMappedRangeValueClamped(
			FVector2D(.1f, 250.f), 
			FVector2D(0.f, 2.f),
			ObjectMass);
	SetRTPCValue(massRTPC, massRTPCValue, 0, FString());
	
	MinVelocityToSpawnImpactSound =	
		FMath::GetMappedRangeValueClamped(
			FVector2D(20.f, 1200.f),
			FVector2D(50.f, 10.f),
			ObjectMass
			);

	MinVelocityDeltaToSpawnImpactSound =
		FMath::GetMappedRangeValueClamped(
			FVector2D(20.f, 1200.f),
			FVector2D(200.f, 50.f),
			ObjectMass
		);
	
	SlideThreshold = FMath::Clamp(FMath::Sqrt(ObjectMass), 1.f, 20.f) * 10.f; 
}

void UPAPhysicsAudioComponent::LoadAkAudioEvents()
{
	TArray<FSoftObjectPath> AssetsToLoad;

	if (!PhysicsActorAudioProperties.ImpactSound.IsNull())
		AssetsToLoad.Add(PhysicsActorAudioProperties.ImpactSound.ToSoftObjectPath());
	
	if (!PhysicsActorAudioProperties.SlideSound.IsNull())
		AssetsToLoad.Add(PhysicsActorAudioProperties.SlideSound.ToSoftObjectPath());
	
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
	ImpactSound = PhysicsActorAudioProperties.ImpactSound.Get();
	SlideSound = PhysicsActorAudioProperties.SlideSound.Get();
	DestructionSound = PhysicsActorAudioProperties.DestructionSound.Get();
	bAkAudioEventsLoaded = true;
	SetAudibilityRange();
}

void UPAPhysicsAudioComponent::SetAudibilityRange()
{
	if (AudibilityRangeSquared < 0) // means we already have it determined
		return;
	TArray<UAkAudioEvent*> associatedEvents;
	associatedEvents.Add(ImpactSound);
	associatedEvents.Add(SlideSound);
	associatedEvents.Add(DestructionSound);
	for (auto& akEvent : associatedEvents)
		if (akEvent != nullptr)
			if (akEvent->MaxAttenuationRadius > AudibilityRangeSquared)
				AudibilityRangeSquared = FMath::Square(akEvent->MaxAttenuationRadius);
}

bool UPAPhysicsAudioComponent::IsAudible() const
{
	if (AudibilityRangeSquared <= 0.f) // no event specified
		return false;
	if (!IsValid(GetWorld()))
		return false;
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
	if (!IsValid(subsystem))
		return false;
	
	const TArray<FVector>& listenersPositions = subsystem->GetListenersPositions();
	if (listenersPositions.IsEmpty())
		return false;
	
	for (const FVector& listenerPosition : listenersPositions)
		if (FVector::DistSquared(listenerPosition, GetComponentLocation()) <= AudibilityRangeSquared)
			return true;
	
	return false;
}

void UPAPhysicsAudioComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bAkAudioEventsLoaded || !IsAudible()) // check if we even bother calculating physics
		return;
	
	FVector currentVelocity = ParentComponent->GetPhysicsLinearVelocity();
	CurrentVelocity = currentVelocity.Size();

	FVector surfaceNormal = Hit.Normal.GetSafeNormal();
	FVector normalComponent = FVector::DotProduct(currentVelocity, surfaceNormal) * surfaceNormal;
	FVector tangentialVelocity = currentVelocity - normalComponent;
	float currentSlideSpeed = tangentialVelocity.Size();
	
	if (Hit.PhysMaterial.Get())
		SetSwitch(UPAFunctionLibrary::GetAkSwitchFromSurface(Hit.PhysMaterial->SurfaceType));
	
	if (CheckVelocityDelta() && ImpactSound != nullptr)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), "Bump", true, false, FLinearColor::Green, .2f);
		LastPlayedVelocity = CurrentVelocity;
		SetVelocityRTPC();
		PostAkEvent(ImpactSound, 0, FOnAkPostEventCallback());
	}
	
	if (currentSlideSpeed > SlideThreshold && SlideSound != nullptr)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), "Slide", true, false, FLinearColor::Blue, .2f);
		SetVelocityRTPC();
		PostAkEvent(SlideSound, 0, FOnAkPostEventCallback());
	}
}

bool UPAPhysicsAudioComponent::CheckVelocityDelta() const
{
	const bool bComparedVelocities = FMath::Abs(CurrentVelocity - LastPlayedVelocity) > MinVelocityDeltaToSpawnImpactSound;
	return CurrentVelocity > MinVelocityToSpawnImpactSound && bComparedVelocities;
}

void UPAPhysicsAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ParentComponent))	
		ParentComponent->OnComponentHit.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}
