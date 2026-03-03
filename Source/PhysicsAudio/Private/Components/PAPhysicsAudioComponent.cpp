// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PAPhysicsAudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PAPhysicsAudioSubsystem.h"
#include "System/PAFunctionLibrary.h"
#include "System/PhysicsAudioSettings.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/Actor.h"

UPAPhysicsAudioComponent::UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	
	ImpactCooldown = 0.f;
	SlideCooldown = 0.f;
	RollCooldown = 0.f;
	bIsSliding = false;
	bIsRolling = false;
	bWasRolling = false;
	TimeSinceLastRollDetection = 0.f;
}

void UPAPhysicsAudioComponent::OnAttachedToPhysicsComponent(UPrimitiveComponent* InPhysicsComponent,
                                                            const FPAPhysicsActorAudioHandle& InAudioProperties, float InMassOverride)
{
	check(InPhysicsComponent);	
	PhysicsActorAudioProperties = InAudioProperties;
	ParentComponent = InPhysicsComponent;
	ParentComponent->OnComponentHit.AddUniqueDynamic(this, &UPAPhysicsAudioComponent::OnComponentHit);
	
	ObjectMassOverride = InMassOverride;
	RTPCAssets = UPAFunctionLibrary::GetRTPC_Assets();
	
	SetMassData();
	LoadAkAudioEvents();
	
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UPAPhysicsAudioComponent::OnDetachedFromPhysicsComponent()
{
	if (IsValid(ParentComponent))
		ParentComponent->OnComponentHit.RemoveAll(this);
	ParentComponent = nullptr;
	
	PrimaryComponentTick.SetTickFunctionEnable(false);	
	
	bAkAudioEventsReady = false;
	// Let GC do it's work
	ImpactSound = nullptr;
	SlideSound = nullptr;
	RollSound = nullptr;
	ProjectileSound = nullptr;
	DestructionSound = nullptr;
}

void UPAPhysicsAudioComponent::OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
                                                                const FVector& InProjectileImpulse)
{
	if (!IsAkEventValidAndAudible(ProjectileSound))
		return;
	
	const float impulseMagnitude = InProjectileImpulse.Size();
	const float projectileRTPCValue = FMath::GetMappedRangeValueClamped(
		FVector2D(50000.f, 500000.f),
		FVector2D(0.f, 100.f),
		impulseMagnitude
		);
	
	SetRTPCValue(RTPCAssets->ProjectileRTPC, FMath::Clamp(projectileRTPCValue, 0.f, 100.f), 0, FString());
	PostAkEvent(ProjectileSound, 0, FOnAkPostEventCallback());
}

void UPAPhysicsAudioComponent::SetMassData()
{
	if (ObjectMassOverride > 0.f)
		ObjectMass = ObjectMassOverride;
	
	// Calculate mass-normalized thresholds
	// Using square root scaling for more natural perception
	ObjectMassSqrt = FMath::Sqrt(ObjectMass);
	SetRTPCValue(RTPCAssets->MassRTPC, FMath::Clamp(ObjectMass, 0.f, 500.f), 0, FString());
	
	// Impact threshold: heavier objects need more force for impact sounds
	ImpactThreshold = FMath::GetMappedRangeValueClamped(
		FVector2D(1.f, 1000.f),
		FVector2D(25.f, 500.f),
		ObjectMass
	);
	
	// Slide threshold: normalize by sqrt(mass) so perceived speed is similar
	SlideThreshold = 50.f * ObjectMassSqrt;
	
	// Roll threshold: angular velocity needed to detect rolling
	RollThreshold = FMath::GetMappedRangeValueClamped(
		FVector2D(1.f, 1000.f),
		FVector2D(5.f, 20.f),
		ObjectMass
	);
	
	// Velocity change needed to trigger impact
	VelocityDeltaThreshold = FMath::GetMappedRangeValueClamped(
		FVector2D(1.f, 1000.f),
		FVector2D(10.f, 100.f),
		ObjectMass
	);
}

float UPAPhysicsAudioComponent::NormalizeByMass(float InValue, float InMass, float InExponent)
{
	// Normalize physics values by mass to achieve perceptual consistency
	if (InMass <= 0.f) 
		return InValue;
	return InValue / FMath::Pow(InMass, InExponent);
}

bool UPAPhysicsAudioComponent::ShouldPlayImpact(float InImpulseMagnitude, float InVelocityMagnitude, float InVelocityDelta) const
{
	const float currentImpactThreshold = bGrounded ? ImpactThreshold * 5.f : ImpactThreshold;
	const float currentVelocityDeltaThreshold = bGrounded ? VelocityDeltaThreshold * 5.f : VelocityDeltaThreshold;
	return ImpactCooldown >= PhysicsAudioSettings::PHYSICS_AUDIO_COOLDOWN_TIME
			&& InVelocityMagnitude > PhysicsAudioSettings::PHYSICS_AUDIO_MIN_VELOCITY
			&& InImpulseMagnitude > currentImpactThreshold
			&& InVelocityDelta > currentVelocityDeltaThreshold;
}

void UPAPhysicsAudioComponent::LoadAkAudioEvents()
{
	/*In order to have small detection sphere radius we need this one as soon as possible, because depending on
	projectile speed the time between detection and projectile hit may be to short for AkEvent to load async*/	
	if (!PhysicsActorAudioProperties.ProjectileSound.IsNull())
		PhysicsActorAudioProperties.ProjectileSound.LoadSynchronous(); 
	ProjectileSound = PhysicsActorAudioProperties.ProjectileSound.Get();
	
	TArray<FSoftObjectPath> assetsToLoadAsync;
	if (!PhysicsActorAudioProperties.ImpactSound.IsNull())
		assetsToLoadAsync.Add(PhysicsActorAudioProperties.ImpactSound.ToSoftObjectPath());	
	if (!PhysicsActorAudioProperties.SlideSound.IsNull())
		assetsToLoadAsync.Add(PhysicsActorAudioProperties.SlideSound.ToSoftObjectPath());
	if (!PhysicsActorAudioProperties.RollSound.IsNull())
		assetsToLoadAsync.Add(PhysicsActorAudioProperties.RollSound.ToSoftObjectPath());
	if (!PhysicsActorAudioProperties.DestructionSound.IsNull())
		assetsToLoadAsync.Add(PhysicsActorAudioProperties.DestructionSound.ToSoftObjectPath());

	if (!assetsToLoadAsync.IsEmpty())
	{
		FStreamableManager& streamable = UAssetManager::GetStreamableManager();
		streamable.RequestAsyncLoad(
			assetsToLoadAsync,
			FStreamableDelegate::CreateUObject(this, &UPAPhysicsAudioComponent::OnAkAudioEventsLoaded)
		);
		return;
	}	
	
	// No assets to load, return to pool
	if (IsValid(GetWorld()))
		if (UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld()))
			subsystem->ReturnPhysicsAudioComponentToPool(ParentComponent);
	DestroyComponent();
}

void UPAPhysicsAudioComponent::OnAkAudioEventsLoaded()
{
	ImpactSound = PhysicsActorAudioProperties.ImpactSound.Get();
	SlideSound = PhysicsActorAudioProperties.SlideSound.Get();
	RollSound = PhysicsActorAudioProperties.RollSound.Get();
	DestructionSound = PhysicsActorAudioProperties.DestructionSound.Get();
	
	bAkAudioEventsReady = true;
}

bool UPAPhysicsAudioComponent::IsAkEventValidAndAudible(const UAkAudioEvent* InEvent) const
{
	if (!IsValid(InEvent))
		return false;
	return DistanceToClosestListenerSquared <= FMath::Square(InEvent->MaxAttenuationRadius);
}

void UPAPhysicsAudioComponent::UpdatePhysicsState(float DeltaTime)
{	
	/* Physics state is updated on tick and on component hit, 
	 *this prevents from update happening twice in one frame*/
	if (bPhysicsStateUpdated)
	{
		bPhysicsStateUpdated = false;
		return;
	}
	// Update velocity tracking
	PreviousVelocity = CurrentVelocity;
	PreviousVelocityMagnitude = CurrentVelocityMagnitude;
	
	CurrentVelocity = ParentComponent->GetPhysicsLinearVelocity();
	CurrentVelocityMagnitude = CurrentVelocity.Size();
	
	// Calculate velocity delta
	VelocityDeltaMagnitude = FMath::Abs(CurrentVelocityMagnitude - PreviousVelocityMagnitude);
	
	// Update grounded state
	if (VelocityDeltaMagnitude >= GroundedThreshold)
		bGrounded = false;
	
	// Update rolling state
	// Get angular velocity
	FVector angularVel = ParentComponent->GetPhysicsAngularVelocityInRadians();
	AngularSpeed = angularVel.Size();
	
	// Calculate change in angular speed
	PreviousAngularSpeed = CurrentAngularSpeed;
	CurrentAngularSpeed = AngularSpeed;
	
	// Determine if rolling (angular velocity above threshold AND on ground)
	bool bShouldBeRolling = bGrounded && AngularSpeed > RollThreshold;
	
	// Rolling cooldown to prevent rapid toggling
	if (bShouldBeRolling)
		TimeSinceLastRollDetection = 0.f;
	else
		TimeSinceLastRollDetection += DeltaTime;
	
	// Only change state after a short cooldown to prevent flickering
	if (TimeSinceLastRollDetection > 0.1f)
		bIsRolling = bShouldBeRolling;
	else
		bIsRolling = true; // Maintain rolling state during cooldown
	
	// Track if rolling state changed for event triggering
	bWasRolling = bIsRolling != bShouldBeRolling ? bIsRolling : bWasRolling;
	
	bPhysicsStateUpdated = true;
}

void UPAPhysicsAudioComponent::UpdateRTPCValues()
{
	if (!RTPCAssets) 
		return;
	
	float slideValue = 0.f;
	
	if (bIsRolling)
		// When rolling, slide component is minimal (just enough to hear rolling texture)
		slideValue = 10.f; // Base rolling slide
	else if (bGrounded && CurrentVelocityMagnitude > 10.f)
	{
		// Not rolling but on ground = sliding
		// Normalize by mass for perceptual consistency
		float normalizedSpeed = NormalizeByMass(CurrentVelocityMagnitude, ObjectMass);
		slideValue = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 500.f),
		FVector2D(0.f, 100.f),
		normalizedSpeed
		);
	}
	if (!FMath::IsNearlyEqual(CurrentSlideRTPCValue, slideValue, .5f)) // Set RTPC only when it changed
	{
		CurrentSlideRTPCValue = slideValue;
		SetRTPCValue(RTPCAssets->SlideRTPC, FMath::Clamp(CurrentSlideRTPCValue, 0.f, 100.f), 0, FString());
	}	
	
	// --- ROLL RTPC (0-100) ---
	float rollValue = 0.f;
	
	if (bIsRolling)
	{
		// Normalize angular speed by object size/mass
		// Larger objects roll slower perceptually for same angular velocity
		float normalizedAngularSpeed = NormalizeByMass(AngularSpeed, ObjectMass, 0.33f);
		rollValue = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 50.f),
		FVector2D(0.f, 100.f),
		normalizedAngularSpeed
		);
	}
	if (!FMath::IsNearlyEqual(CurrentRollRTPCValue, rollValue, .5f)) // Set RTPC only when it changed
	{
		CurrentRollRTPCValue = rollValue;
		SetRTPCValue(RTPCAssets->RollRTPC, FMath::Clamp(CurrentRollRTPCValue, 0.f, 100.f), 0, FString());
	}	
}

void UPAPhysicsAudioComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bAkAudioEventsReady || !IsAkEventValidAndAudible(ImpactSound))
		return;
		
	// Update physics state first to get latest velocities
	UpdatePhysicsState(0.f); // DeltaTime not needed for immediate update
	
	// Set surface type switch if physical material exists
	if (Hit.PhysMaterial.Get())
		SetSwitch(UPAFunctionLibrary::GetAkSwitchFromSurface(Hit.PhysMaterial->SurfaceType));
	
	// --- IMPACT DETECTION ---
	float impulseMagnitude = NormalImpulse.Size();
	
	if (ShouldPlayImpact(impulseMagnitude, CurrentVelocityMagnitude, VelocityDeltaMagnitude))
	{
		ImpactCooldown = 0.f;
		
		// Calculate impact RTPC (0-100) based on impulse and mass
		// Normalize impulse by mass for consistent feel
		float normalizedImpulse = impulseMagnitude / ObjectMassSqrt;
		float impactRTPCValue = FMath::GetMappedRangeValueClamped(
		FVector2D(50.f, 2000.f),
		FVector2D(0.f, 100.f),
		normalizedImpulse
		);
		
		SetRTPCValue(RTPCAssets->ImpactRTPC, FMath::Clamp(impactRTPCValue, 0.f, 100.f), 0, FString());
		PostAkEvent(ImpactSound, 0, FOnAkPostEventCallback());
		FLinearColor debugColor = bIsSliding ? FLinearColor::White : FLinearColor::Black;
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 100.f,12,debugColor, .5f);
	}
	bGrounded = true;	
}

void UPAPhysicsAudioComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bAkAudioEventsReady)
		return;
	if (!IsValid(ParentComponent))
	{
		UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
		if (IsValid(subsystem))
		{
			subsystem->ReturnOrphanedAudioComponentToPool(this);
			return;
		}
		DestroyComponent();
		return;
	}
	// Update cooldowns
	if (ImpactCooldown < PhysicsAudioSettings::PHYSICS_AUDIO_COOLDOWN_TIME)
		ImpactCooldown += DeltaTime;
	if (SlideCooldown < PhysicsAudioSettings::PHYSICS_AUDIO_COOLDOWN_TIME)
		SlideCooldown += DeltaTime;
	if (RollCooldown < PhysicsAudioSettings::PHYSICS_AUDIO_COOLDOWN_TIME)
		RollCooldown += DeltaTime;
	
	// Check if we even bother
	bSlideAudible = IsAkEventValidAndAudible(SlideSound);
	bRollAudible = IsAkEventValidAndAudible(RollSound);
	if (!bSlideAudible && !bRollAudible)
		return;
	// Update physics state
	UpdatePhysicsState(DeltaTime);
	// Update RTPCs for continuous sound changes
	UpdateRTPCValues();

	FString result = bGrounded ? TEXT("Grounded") : TEXT("");
	FLinearColor debugColor = bGrounded ? FLinearColor::Green : FLinearColor::Red;
	UKismetSystemLibrary::DrawDebugString(GetWorld(), GetComponentLocation(), result, 0, debugColor);

	if (bSlideAudible)
	{
		// Trigger slide sound when sliding starts or when sliding continues with cooldown
		bool bShouldBeSliding = bGrounded && !bIsRolling && CurrentVelocityMagnitude > 50.f;

		if (bShouldBeSliding && SlideCooldown >= PhysicsAudioSettings::PHYSICS_AUDIO_COOLDOWN_TIME)
		{
			SlideCooldown = 0.f;
			bIsSliding = true;

			// Slide RTPC already updated in UpdateRTPCValues()
			PostAkEvent(SlideSound, 0, FOnAkPostEventCallback());
			UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 75.f, 12, FLinearColor::Blue);
		}
		else if (!bShouldBeSliding)
			bIsSliding = false;
	}
	
	if (bRollAudible && bIsRolling && RollCooldown >= PhysicsAudioSettings::PHYSICS_AUDIO_COOLDOWN_TIME)
	{
		RollCooldown = 0.f;
		PostAkEvent(RollSound, 0, FOnAkPostEventCallback());
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 50.f, 12, FLinearColor::Yellow);
	}
}

void UPAPhysicsAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ParentComponent))
		ParentComponent->OnComponentHit.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}