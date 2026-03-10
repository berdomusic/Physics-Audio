// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PAPhysicsAudioComponent.h"

#include "AkGameplayStatics.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PAPhysicsAudioSubsystem.h"
#include "System/PAFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/Actor.h"

UPAPhysicsAudioComponent::UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	bAutoActivate = false;
	
	bIsSliding = false;
	bIsRolling = false;
	TimeSinceLastRollDetection = 0.f;
	
	AudioEvents.SetNum(static_cast<int32>(EPAEventType::Count));
	AudioEventsSoftRefs.SetNum(static_cast<int32>(EPAEventType::Count));
}

void UPAPhysicsAudioComponent::TryReturnComponentToPool()
{
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
	if (IsValid(subsystem))
		subsystem->ReturnPhysicsAudioObjectToPool(nullptr,this, false);
	else
		DestroyComponent();
}

void UPAPhysicsAudioComponent::OnAttachedToSimulatingComponent(UPrimitiveComponent* InComponent,
                                                               const FPAPhysicsActorAudioProperties& InAudioProperties)
{
	ensure(InComponent);
	if(!InComponent->IsSimulatingPhysics())
	{
		UE_LOG(LogTemp, Error, TEXT("Attempting to attach to non simulating primitive, abort"));
		TryReturnComponentToPool();
		return;
	}
	OnAttachedToComponent_Internal(InComponent, InAudioProperties);
}

void UPAPhysicsAudioComponent::OnAttachedToNonSimulatingComponent(UPrimitiveComponent* InComponent,
                                                                  const FPAPhysicsActorAudioProperties& InAudioProperties)
{
	ensure(InComponent);
	if(InAudioProperties.ObjectMassOverride <= 0.f)
	{
		UE_LOG(LogTemp, Error, TEXT("No mass override set for non simulating primitive, abort"));
		TryReturnComponentToPool();
		return;
	}
	OnAttachedToComponent_Internal(InComponent, InAudioProperties);
}

void UPAPhysicsAudioComponent::OnDetachedFromPhysicsComponent()
{
	Deactivate();
}

void UPAPhysicsAudioComponent::OnParentDestroyed()
{
	if (!IsValid(GetWorld()) || !IsAkEventValidAndAudible(GetAkEventByType(EPAEventType::Destruction)))
		return;
	// Posting at location so we can immediately reuse this component or return it to pool
	UAkGameplayStatics::PostEventAtLocation(
		GetAkEventByType(EPAEventType::Destruction), 
		GetComponentLocation(),
		GetComponentRotation(),
		GetWorld()
		);	
}

void UPAPhysicsAudioComponent::OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
                                                                const FVector& InProjectileImpulse)
{
	if (!IsAkEventValidAndAudible(GetAkEventByType(EPAEventType::Projectile)))
		return;
	
	const float impulseMagnitude = InProjectileImpulse.Size();
	const float projectileRTPCValue = FMath::GetMappedRangeValueClamped(
		FVector2D(50000.f, 500000.f),
		FVector2D(0.f, 100.f),
		impulseMagnitude
		);
	
	SetRTPCValue(RTPCAssets->ProjectileRTPC, FMath::Clamp(projectileRTPCValue, 0.f, 100.f), 0, FString());
	PostAkEvent(GetAkEventByType(EPAEventType::Projectile), 0, FOnAkPostEventCallback());
}

void UPAPhysicsAudioComponent::OnAttachedToComponent_Internal(UPrimitiveComponent* InComponent,
	const FPAPhysicsActorAudioProperties& InAudioProperties)
{
	ParentComponent = InComponent;
	PhysicsActorAudioProperties = InAudioProperties;
	if (!UPAFunctionLibrary::IsAudioHandleNotEmpty(PhysicsActorAudioProperties))
	{
		UE_LOG(LogTemp, Error, TEXT("Audio handle empty, abort"));
		TryReturnComponentToPool();
		return;
	}
	
	ObjectMassOverride = InAudioProperties.ObjectMassOverride;
	
	RTPCAssets = UPAFunctionLibrary::GetRTPC_Assets();
	
	SetMassData();
	SetCooldownVariables();
	LoadAkAudioEvents();
}

void UPAPhysicsAudioComponent::SetMassData()
{
	if (ObjectMassOverride > 0.f)
		ObjectMass = ObjectMassOverride;
	else
		ObjectMass = ParentComponent->GetMass();
	ensure(ObjectMass > 0.f);
	
	SetRTPCValue(RTPCAssets->MassRTPC, FMath::Clamp(ObjectMass, 0.f, 500.f), 0, FString());
	
	// Desired minimum impact velocity to produce sound
	float minVelocity = FMath::GetMappedRangeValueClamped(
		FVector2D(1.f, 500.f), 
		FVector2D(1.2f, .8f),
		ObjectMass
	);

	// Convert velocity to impulse threshold
	ImpactThreshold = ObjectMass * minVelocity;
	
	// Roll threshold: angular velocity needed to detect rolling
	RollThreshold = FMath::GetMappedRangeValueClamped(
		FVector2D(1.f, 1000.f),
		FVector2D(5.f, 20.f),
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

bool UPAPhysicsAudioComponent::ShouldPlayImpact(float InImpulseMagnitude) const
{
	const float currentImpactThreshold = bGrounded ? ImpactThreshold * ObjectMass : ImpactThreshold;
	return CurrentImpactCooldown >= ImpactCooldownThreshold
			&& CurrentVelocityMagnitude > PhysicsAudioSettings::PHYSICS_AUDIO_MIN_VELOCITY
			&& InImpulseMagnitude > currentImpactThreshold;
}

void UPAPhysicsAudioComponent::LoadAkAudioEvents()
{
	AudioEventsSoftRefs.Reset();
	AudioEventsSoftRefs.SetNum(static_cast<int32>(EPAEventType::Count));
	
	AudioEventsSoftRefs[static_cast<int32>(EPAEventType::Impact)] = PhysicsActorAudioProperties.ImpactSound;
	AudioEventsSoftRefs[static_cast<int32>(EPAEventType::Slide)] = PhysicsActorAudioProperties.SlideSound;	
	AudioEventsSoftRefs[static_cast<int32>(EPAEventType::Roll)] = PhysicsActorAudioProperties.RollSound;	
	AudioEventsSoftRefs[static_cast<int32>(EPAEventType::Projectile)] = PhysicsActorAudioProperties.ProjectileSound;
	AudioEventsSoftRefs[static_cast<int32>(EPAEventType::Destruction)] = PhysicsActorAudioProperties.DestructionSound;
	
	TArray<FSoftObjectPath> assetsToLoadAsync;

	for (int32 i = 0; i < AudioEventsSoftRefs.Num(); ++i)
	{
		FPAPhysicsAudioEvent& currentAudioEventSoft = AudioEventsSoftRefs[i];
		if (currentAudioEventSoft.AkEventSoft.IsNull())
			continue;
		if (currentAudioEventSoft.bLoadSynchronously)
		{
			currentAudioEventSoft.AkEventSoft.LoadSynchronous();
			AudioEvents[i] = currentAudioEventSoft.AkEventSoft.Get();
		}
		else
			assetsToLoadAsync.Add(currentAudioEventSoft.AkEventSoft.ToSoftObjectPath());
	}
	
	if (!assetsToLoadAsync.IsEmpty())
	{
		FStreamableManager& streamable = UAssetManager::GetStreamableManager();
		streamable.RequestAsyncLoad(
			assetsToLoadAsync,
			FStreamableDelegate::CreateUObject(this, &UPAPhysicsAudioComponent::OnAkAudioEventsLoaded)
		);
		return;
	}
	OnAkAudioEventsLoaded();
}

void UPAPhysicsAudioComponent::OnAkAudioEventsLoaded()
{
	if (!AudioEventsSoftRefs.IsEmpty())
	{
		for (int32 i = 0; i < AudioEventsSoftRefs.Num(); ++i)
		{
			FPAPhysicsAudioEvent& currentAudioEventSoft = AudioEventsSoftRefs[i];
			if (currentAudioEventSoft.AkEventSoft.IsNull())
				continue;
			if (!currentAudioEventSoft.bLoadSynchronously)
				AudioEvents[i] = currentAudioEventSoft.AkEventSoft.Get();			
		}
	}	
	Activate();
}

bool UPAPhysicsAudioComponent::IsAkEventValidAndAudible(const UAkAudioEvent* InEvent) const
{
	if (!IsValid(InEvent))
		return false;
	return DistanceToClosestListenerSquared <= FMath::Square(InEvent->MaxAttenuationRadius);
}

void UPAPhysicsAudioComponent::SetCooldownVariables()
{
	ImpactCooldownThreshold = PhysicsActorAudioProperties.ImpactSound.CooldownThreshold;
	CurrentImpactCooldown = ImpactCooldownThreshold;
	SlideCooldownThreshold = PhysicsActorAudioProperties.SlideSound.CooldownThreshold;
	CurrentSlideCooldown = SlideCooldownThreshold;
	RollCooldownThreshold = PhysicsActorAudioProperties.RollSound.CooldownThreshold;
	CurrenRollCooldown = RollCooldownThreshold;
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
	PreviousVelocityMagnitude = CurrentVelocityMagnitude;
	CurrentVelocityMagnitude = ParentComponent->GetPhysicsLinearVelocity().Size();
	
	// Calculate velocity delta
	const float velocityMagnitudeDelta = FMath::Abs(CurrentVelocityMagnitude - PreviousVelocityMagnitude);
	
	// Update grounded state
	if (velocityMagnitudeDelta >= GroundedThreshold)
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
		bIsRolling = bGrounded; // Maintain rolling state during cooldown if grounded
	
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
	if (!FMath::IsNearlyEqual(PreviousSlideRTPCValue, slideValue, PhysicsAudioSettings::PHYSICS_AUDIO_RTPC_TOLERANCE)) // Set RTPC only when it changed
	{
		PreviousSlideRTPCValue = slideValue;
		SetRTPCValue(RTPCAssets->SlideRTPC, FMath::Clamp(PreviousSlideRTPCValue, 0.f, 100.f), 0, FString());
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
	if (!FMath::IsNearlyEqual(PreviousRollRTPCValue, rollValue, PhysicsAudioSettings::PHYSICS_AUDIO_RTPC_TOLERANCE)) // Set RTPC only when it changed
	{
		PreviousRollRTPCValue = rollValue;
		SetRTPCValue(RTPCAssets->RollRTPC, FMath::Clamp(PreviousRollRTPCValue, 0.f, 100.f), 0, FString());
	}	
}

void UPAPhysicsAudioComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!IsAkEventValidAndAudible(GetAkEventByType(EPAEventType::Impact)))
		return;
		
	// Update physics state first to get latest velocities
	UpdatePhysicsState(0.f); // DeltaTime not needed for immediate update
	
	// Set surface type switch if physical material exists
	if (Hit.PhysMaterial.Get())
		SetSwitch(UPAFunctionLibrary::GetAkSwitchFromSurface(Hit.PhysMaterial->SurfaceType));
	
	// Want stronger impulse if wasn't grounded before
	float impulseMagnitude = NormalImpulse.Size() * (bGrounded ? 1.f : 2.f);
	
	if (ShouldPlayImpact(impulseMagnitude))
	{
		CurrentImpactCooldown = 0.f;
		
		// Normalize impulse by mass for consistent feel
		float normalizedImpulse = impulseMagnitude / ObjectMass;
		float impactRTPCValue = FMath::GetMappedRangeValueClamped(
		FVector2D(50.f, 2000.f),
		FVector2D(0.f, 100.f),
		normalizedImpulse
		);
		
		if (!FMath::IsNearlyEqual(PreviousImpactRTPCValue, impactRTPCValue, PhysicsAudioSettings::PHYSICS_AUDIO_RTPC_TOLERANCE)) // Set RTPC only when it changed
		{
			PreviousImpactRTPCValue = impactRTPCValue;
			SetRTPCValue(RTPCAssets->ImpactRTPC, FMath::Clamp(PreviousImpactRTPCValue, 0.f, 100.f), 0, FString());
		}
		PostAkEvent(GetAkEventByType(EPAEventType::Impact), 0, FOnAkPostEventCallback());
		FLinearColor debugColor = bGrounded ? FLinearColor::White : FLinearColor::Black;
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 100.f,12,debugColor, .5f);
	}
	bGrounded = true;	
}

void UPAPhysicsAudioComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	ParentComponent->OnComponentHit.AddUniqueDynamic(this, &UPAPhysicsAudioComponent::OnComponentHit);
}

void UPAPhysicsAudioComponent::Deactivate()
{
	PrimaryComponentTick.SetTickFunctionEnable(false);
	if (IsValid(ParentComponent))
		ParentComponent->OnComponentHit.RemoveAll(this);
	
	// Let GC do it's work
	ParentComponent = nullptr;	
	AudioEvents.Empty();
	AudioEvents.SetNum(static_cast<int32>(EPAEventType::Count));
	Super::Deactivate();
}

void UPAPhysicsAudioComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsValid(ParentComponent))
	{
		TryReturnComponentToPool();
		return;
	}
	// Update cooldowns
	if (CurrentImpactCooldown < ImpactCooldownThreshold)
		CurrentImpactCooldown += DeltaTime;
	if (CurrentSlideCooldown < SlideCooldownThreshold)
		CurrentSlideCooldown += DeltaTime;
	if (CurrenRollCooldown < RollCooldownThreshold)
		CurrenRollCooldown += DeltaTime;
	
	// Check if we even bother
	bSlideAudible = IsAkEventValidAndAudible(GetAkEventByType(EPAEventType::Slide));
	bRollAudible = IsAkEventValidAndAudible(GetAkEventByType(EPAEventType::Roll));
	if (!bSlideAudible && !bRollAudible)
		return;
	// Update physics state
	UpdatePhysicsState(DeltaTime);
	// Update RTPCs for continuous sound changes
	UpdateRTPCValues();
	
	if (bGrounded)
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 50.f, 0, FLinearColor::Green);

	if (bSlideAudible)
	{
		// Trigger slide sound when sliding starts or when sliding continues with cooldown
		bool bShouldBeSliding = bGrounded && !bIsRolling && CurrentVelocityMagnitude > 50.f;

		if (bShouldBeSliding && CurrentSlideCooldown >= SlideCooldownThreshold)
		{
			CurrentSlideCooldown = 0.f;
			bIsSliding = true;

			// Slide RTPC already updated in UpdateRTPCValues()
			PostAkEvent(GetAkEventByType(EPAEventType::Slide), 0, FOnAkPostEventCallback());
			UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 75.f, 12, FLinearColor::Blue);
		}
		else if (!bShouldBeSliding)
			bIsSliding = false;
	}
	
	if (bRollAudible && bIsRolling && CurrenRollCooldown >= RollCooldownThreshold)
	{
		CurrenRollCooldown = 0.f;
		PostAkEvent(GetAkEventByType(EPAEventType::Roll), 0, FOnAkPostEventCallback());
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 50.f, 12, FLinearColor::Yellow);
	}
}

void UPAPhysicsAudioComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (IsValid(ParentComponent))
		ParentComponent->OnComponentHit.RemoveAll(this);
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}
