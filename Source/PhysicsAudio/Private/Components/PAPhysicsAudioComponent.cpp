// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PAPhysicsAudioComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PAPhysicsAudioSubsystem.h"
#include "System/PAFunctionLibrary.h"
#include "System/PhysicsAudioSettings.h"

UPAPhysicsAudioComponent::UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPAPhysicsAudioComponent::OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent,
                                                            const FPAPhysicsActorAudioHandle& InAudioProperties, float InMassOverride)
{
	check(InPhysicsComponent);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	PhysicsActorAudioProperties = InAudioProperties;
	ParentComponent = InPhysicsComponent;
	ParentComponent->OnComponentHit.AddUniqueDynamic(this, & UPAPhysicsAudioComponent::OnComponentHit);
	ObjectMassOverride = InMassOverride;
	
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
	ProjectileSound = nullptr;
	DestructionSound = nullptr;
}

void UPAPhysicsAudioComponent::SetVelocityRTPC(bool bInAccountForDelta) const
{
	//take into account if object slows down
	bool bAccountForDelta = bInAccountForDelta 
		&& PreviousVelocity > CurrentVelocity;
	
	const float mappedVelocity = 
		FMath::GetMappedRangeValueClamped(
			FVector2D(50.f, 500.f),
			FVector2D(.1f,1.f),
			CurrentVelocity);
	
	const float mappedDelta = bAccountForDelta ? 
		FMath::GetMappedRangeValueClamped(
			FVector2D (.1f, 10.f),
			FVector2D(.1f,1.f),
			CurrentVelocityDelta
			) : 0;
	
	const float resultRTPC = FMath::Clamp(mappedVelocity + mappedDelta, 0.f, 1.f);
	
	SetRTPCValue(UPAFunctionLibrary::GetRTPC_Assets()->VelocityRTPC, resultRTPC, 0, FString());
	UKismetSystemLibrary::DrawDebugString(
			GetWorld(), 
			GetComponentLocation(), 
			FString::SanitizeFloat(CurrentVelocityDelta), 
			0,FLinearColor::Green,
			.5f
			);
}

void UPAPhysicsAudioComponent::OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
                                                                const FVector& InProjectileImpulse)
{
	if (!bAkAudioEventsLoaded || !IsAudible()) // check if we even bother playing sound
		return;
	if (ProjectileSound == nullptr)
		return;
	//UKismetSystemLibrary::PrintString(GetWorld(), "Projectile", true, false, FLinearColor::Red, .2f);	
	const float speedRTPC_Value = 
		FMath::GetMappedRangeValueClamped(
			FVector2D(50000.f, 300000.f),
			FVector2D(0.f, 100.f),
			InProjectileImpulse.Size());
	SetRTPCValue(UPAFunctionLibrary::GetRTPC_Assets()->ProjectileRTPC, speedRTPC_Value, 0, FString());
	PostAkEvent(ProjectileSound, 0, FOnAkPostEventCallback());
}

void UPAPhysicsAudioComponent::SetMassData()
{
	if (ObjectMassOverride > 0.f)
		ObjectMass = ObjectMassOverride;
	else
		ObjectMass = ParentComponent->GetMass();
	
	const float massRTPCValue = 
		FMath::GetMappedRangeValueClamped(
			FVector2D(.1f, 250.f), 
			FVector2D(0.f, 2.f),
			ObjectMass);
	SetRTPCValue(UPAFunctionLibrary::GetRTPC_Assets()->MassRTPC, massRTPCValue, 0, FString());

	MinVelocityDeltaToSpawnImpactSound =
		/*FMath::GetMappedRangeValueClamped(
		FVector2D(100.f, 1000.f),
		FVector2D(100.f, 500.f),
		ObjectMass
	);*/
		FMath::GetMappedRangeValueClamped(
			FVector2D(20.f, 1000.f),
			FVector2D(200.f, 100.f),
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
	
	if (!PhysicsActorAudioProperties.ProjectileSound.IsNull())
		AssetsToLoad.Add(PhysicsActorAudioProperties.ProjectileSound.ToSoftObjectPath());
	
	if (!PhysicsActorAudioProperties.DestructionSound.IsNull())
		AssetsToLoad.Add(PhysicsActorAudioProperties.DestructionSound.ToSoftObjectPath());

	if (AssetsToLoad.IsEmpty())
	{
		if (IsValid(GetWorld()))
			if (UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld()))
			{
				subsystem->ReturnPhysicsAudioComponentToPool(ParentComponent);
				return;
			}
		DestroyComponent();
	}
		

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
	ProjectileSound = PhysicsActorAudioProperties.ProjectileSound.Get();
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
	associatedEvents.Add(ProjectileSound);
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
	
	//float impulseMagnitude = NormalImpulse.Size();
	
	FVector currentVelocity = ParentComponent->GetPhysicsLinearVelocity();
	PreviousVelocity = CurrentVelocity;
	CurrentVelocity = currentVelocity.Size();
	CurrentVelocityDelta = FMath::Abs(CurrentVelocity - PreviousVelocity);

	FVector surfaceNormal = Hit.Normal.GetSafeNormal();
	FVector normalComponent = FVector::DotProduct(currentVelocity, surfaceNormal) * surfaceNormal;
	FVector tangentialVelocity = currentVelocity - normalComponent;
	float currentSlideSpeed = tangentialVelocity.Size();
	
	if (Hit.PhysMaterial.Get())
		SetSwitch(UPAFunctionLibrary::GetAkSwitchFromSurface(Hit.PhysMaterial->SurfaceType));
	
	if (SlideSound != nullptr && currentSlideSpeed > SlideThreshold)
	{
		bIsSliding = true;
		//UKismetSystemLibrary::PrintString(GetWorld(), "Slide", true, false, FLinearColor::Blue, .2f);
		SetVelocityRTPC(false);
		PostAkEvent(SlideSound, 0, FOnAkPostEventCallback());
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 75.f,12,FLinearColor::Blue);
	}
	else
		bIsSliding = false;
	
	if (ImpactSound != nullptr && //impulseMagnitude > MinVelocityDeltaToSpawnImpactSound && ImpactCooldown >= .2f)
		CheckVelocityDelta())
	{
		ImpactCooldown = 0.f;
		//PreviousVelocity = CurrentVelocity;
		SetVelocityRTPC(true);
		PostAkEvent(ImpactSound, 0, FOnAkPostEventCallback());
		FLinearColor debugColor = bIsSliding ? FLinearColor::White : FLinearColor::Black;
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 100.f,12,debugColor);
	}
	else
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 90.f,12,FLinearColor::Red);
}

bool UPAPhysicsAudioComponent::CheckVelocityDelta()
{
	const float deltaThreshold = bIsSliding ? MinVelocityDeltaToSpawnImpactSound * 2.f : MinVelocityDeltaToSpawnImpactSound;
	return CurrentVelocity > PhysicsAudioSettings::PHYSICS_AUDIO_ACTIVATION_VELOCITY
		&& CurrentVelocityDelta > deltaThreshold;
}

void UPAPhysicsAudioComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (ImpactCooldown < .2f)
		ImpactCooldown += DeltaTime;
}

void UPAPhysicsAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ParentComponent))	
		ParentComponent->OnComponentHit.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}
