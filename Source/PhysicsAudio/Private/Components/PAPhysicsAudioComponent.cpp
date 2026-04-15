// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PAPhysicsAudioComponent.h"

#include "AkGameplayStatics.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Engine/AssetManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PAPhysicsAudioComponentSubsystem.h"
#include "System/PAFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "System/PhysicsAudioSettings.h"
#include "System/PAGameReferencesSubsystem.h"

UPAPhysicsAudioComponent::UPAPhysicsAudioComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	bAutoActivate = false;
		
	AudioEvents.SetNum(static_cast<int32>(EPAEventType::Count));
	AudioEventsSoftRefs.SetNum(static_cast<int32>(EPAEventType::Count));
}

void UPAPhysicsAudioComponent::TryReturnComponentToPool()
{
	UPAPhysicsAudioComponentSubsystem* subsystem = UPAPhysicsAudioComponentSubsystem::Get(GetWorld());
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

void UPAPhysicsAudioComponent::DeactivatePhysicsAudioComponent()
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

void UPAPhysicsAudioComponent::OnPickup_Implementation(AActor* InInstigator)
{
	bGrounded = false;
	bPickedUp = true;
}

void UPAPhysicsAudioComponent::OnDrop_Implementation(AActor* InInstigator)
{
	bPickedUp = false;
}

void UPAPhysicsAudioComponent::OnPhysicsActorHit_Implementation(FVector NormalImpulse, const FHitResult& Hit)
{
	OnComponentHit(nullptr, nullptr, nullptr, NormalImpulse, Hit);
}

void UPAPhysicsAudioComponent::OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit,
                                                                const FVector& InProjectileImpulse)
{
	if (!IsAkEventValidAndAudible(GetAkEventByType(EPAEventType::Projectile)))
		return;
	
	const float projectileRTPCValue = FMath::GetMappedRangeValueClamped(
		FVector2D(50000.f, 500000.f),
		FVector2D(0.f, 100.f),
		InProjectileImpulse.Size()
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
	
	if (UDataAsset* dataAsset = UPAGameReferencesSubsystem::Get().GetRTPCsDataAsset())
		if (UPAPhysicsRTPCs* rtpcAssets = Cast<UPAPhysicsRTPCs>(dataAsset))
			RTPCAssets = rtpcAssets;
		else
		{
			checkNoEntry()
			return;
		}
	bIsSliding = false;
	bIsRolling = false;
	
	SetMassData();
	SetCooldownVariables();
	Activate();
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

	// Minimum impact VELOCITY needed to produce sound (not impulse)
	// Light objects: ~2.5 m/s, Heavy objects: ~0.8 m/s
	ImpactVelocityThreshold = FMath::GetMappedRangeValueClamped(
		FVector2D(1.f, 1000.f),
		FVector2D(2.5f, .8f),
		ObjectMass
	);;
    
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
	if (CurrentImpactCooldown < ImpactCooldownThreshold)
		return false;
		
	if (CurrentVelocityMagnitude <= UPhysicsAudioSettings::GetMinVelocity())
		return false;
		
	float velocityThreshold = bGrounded ? ImpactVelocityThreshold * ObjectMass : ImpactVelocityThreshold;
	bool success = InImpulseMagnitude > velocityThreshold;
	if (!success)
		return false;
	return success;
}

void UPAPhysicsAudioComponent::StopContinuousSound(TArray<EPAEventType> InEventTypes)
{
	if (InEventTypes.IsEmpty())
		return;
	for (EPAEventType eventType : InEventTypes)
	{
		UAkAudioEvent* akEvent = GetAkEventByType(eventType);
		if (!IsValid(akEvent) || !akEvent->IsInfinite)
			continue;
		switch (eventType)
		{
		case EPAEventType::Slide:
			PostAkEvent(UPAFunctionLibrary::GetStopContinousSoundEvent(EPAEventType::Slide), 0,
							FOnAkPostEventCallback());
			bSlideLoopPosted = false;
			break;
			
		case EPAEventType::Roll:
			PostAkEvent(UPAFunctionLibrary::GetStopContinousSoundEvent(EPAEventType::Roll), 0,
							FOnAkPostEventCallback());
			bRollLoopPosted = false;
			break;
			
			default:
			break;
		}
	}
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
	
	FStreamableDelegate delegate = FStreamableDelegate::CreateUObject(
		this, 
		&UPAPhysicsAudioComponent::OnAkAudioEventsLoaded
	);
	UPAFunctionLibrary::LoadEventsFromHandle(PhysicsActorAudioProperties, delegate);
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
			AudioEvents[i] = currentAudioEventSoft.AkEventSoft.Get();			
		}
	}
	SetInfiniteEventsVariables();
}

void UPAPhysicsAudioComponent::SetInfiniteEventsVariables(TArray<EPAEventType> InEventTypes)
{
	for (EPAEventType eventType : InEventTypes)
	{
		UAkAudioEvent* akEvent = GetAkEventByType(eventType);
		if (!IsValid(akEvent))
			continue;
		switch (eventType)
		{
		case EPAEventType::Slide:
			bSlideInfinite = akEvent->IsInfinite;
			break;

		case EPAEventType::Roll:
			bRollInfinite = akEvent->IsInfinite;
			break;

		default:
			break;
		}
	}
		
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
	// Update velocity tracking
	PreviousVelocityMagnitude = CurrentVelocityMagnitude;
	CurrentVelocityMagnitude = ParentComponent->GetPhysicsLinearVelocity().Size();	
	
	if (CurrentVelocityMagnitude < UPhysicsAudioSettings::GetMinVelocity())
	{
		bIsRolling = false;
		bIsSliding = false;
		return;
	}
	// Update grounded state
	if (bGrounded)
	{
		// Calculate velocity delta
		const float velocityMagnitudeDelta = FMath::Abs(CurrentVelocityMagnitude - PreviousVelocityMagnitude);
		if (velocityMagnitudeDelta >= GroundedThreshold)
			bGrounded = false;
	}
	
	// Update rolling state
	// Get angular velocity
	FVector angularVel = ParentComponent->GetPhysicsAngularVelocityInRadians();
	
	// Calculate change in angular speed
	PreviousAngularSpeed = CurrentAngularSpeed;
	CurrentAngularSpeed = angularVel.Size();
	
	// Determine if rolling (angular velocity above threshold AND on ground)
	bool bShouldBeRolling = bGrounded && !bPickedUp && CurrentAngularSpeed > RollThreshold;
	
	// Rolling cooldown to prevent rapid toggling
	if (bShouldBeRolling)
		TimeSinceLastRollDetection = 0.f;
	else
		TimeSinceLastRollDetection += DeltaTime;
	
	// Only change state after a short cooldown to prevent flickering
	if (TimeSinceLastRollDetection > .1f)
		bIsRolling = bShouldBeRolling;
	else
		bIsRolling = bGrounded; // Maintain rolling state during cooldown if grounded
	
	bIsSliding = bGrounded && !bPickedUp && !bIsRolling && CurrentVelocityMagnitude > UPhysicsAudioSettings::GetMinVelocity();
}

void UPAPhysicsAudioComponent::UpdateRTPCValues()
{
	if (!RTPCAssets) 
		return;
	
	float slideValue = 0.f;
	
	if (bIsRolling)
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
	if (!FMath::IsNearlyEqual(PreviousSlideRTPCValue, slideValue, UPhysicsAudioSettings::GetRTPCTolerance())) // Set RTPC only when it changed
	{
		PreviousSlideRTPCValue = slideValue;
		SetRTPCValue(RTPCAssets->SlideRTPC, FMath::Clamp(PreviousSlideRTPCValue, 0.f, 100.f), 100, FString());
	}	
	
	// --- ROLL RTPC (0-100) ---
	float rollValue = 0.f;
	
	if (bIsRolling)
	{
		// Normalize angular speed by object size/mass
		// Larger objects roll slower perceptually for same angular velocity
		float normalizedAngularSpeed = NormalizeByMass(CurrentAngularSpeed, ObjectMass, 0.33f);
		rollValue = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 50.f),
		FVector2D(0.f, 100.f),
		normalizedAngularSpeed
		);
	}
	if (!FMath::IsNearlyEqual(PreviousRollRTPCValue, rollValue, UPhysicsAudioSettings::GetRTPCTolerance())) // Set RTPC only when it changed
	{
		PreviousRollRTPCValue = rollValue;
		SetRTPCValue(RTPCAssets->RollRTPC, FMath::Clamp(PreviousRollRTPCValue, 0.f, 100.f), 100, FString());
	}	
}

void UPAPhysicsAudioComponent::HandleStoppingLoops()
{
	if (bRollLoopPosted && (!bRollAudible || !bIsRolling))
		StopContinuousSound({ EPAEventType::Roll });
	if (bSlideLoopPosted && (!bSlideAudible || !bIsSliding))
		StopContinuousSound({ EPAEventType::Slide });
}

void UPAPhysicsAudioComponent::HandlePlayingContinuousSounds()
{
	HandlePlayingSlide();	
	HandlePlayingRoll();
}

void UPAPhysicsAudioComponent::HandlePlayingSlide()
{
	if (bSlideAudible && bIsSliding)
	{
		if (bSlideInfinite)
		{
			if (!bSlideLoopPosted)
			{			
				PostAkEvent(GetAkEventByType(EPAEventType::Slide), 0, FOnAkPostEventCallback());
				bSlideLoopPosted = true;
			}
		}			
		else if (CurrentSlideCooldown >= SlideCooldownThreshold)
		{
			CurrentSlideCooldown = 0.f;
			PostAkEvent(GetAkEventByType(EPAEventType::Slide), 0, FOnAkPostEventCallback());
		}
	}
}

void UPAPhysicsAudioComponent::HandlePlayingRoll()
{
	if (bRollAudible && bIsRolling)
	{
		if (bRollInfinite)
		{
			if (!bRollLoopPosted)
			{			
				PostAkEvent(GetAkEventByType(EPAEventType::Roll), 0, FOnAkPostEventCallback());
				bRollLoopPosted = true;
			}
		}			
		else if (CurrenRollCooldown >= RollCooldownThreshold)
		{
			CurrenRollCooldown = 0.f;
			PostAkEvent(GetAkEventByType(EPAEventType::Roll), 0, FOnAkPostEventCallback());
		}
	}
}

void UPAPhysicsAudioComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!IsAkEventValidAndAudible(GetAkEventByType(EPAEventType::Impact)))
	{
		bGrounded = true;
		return;
	}		
	
	// Set surface type switch if physical material exists
	if (Hit.PhysMaterial.Get())
		SetSwitch(UPAFunctionLibrary::GetAkSwitchFromSurface(Hit.PhysMaterial->SurfaceType));
	
	// Want stronger impulse if wasn't grounded before
	float impulseMagnitude = NormalImpulse.Size() * (bGrounded ? 1.f : 2.f);
	float normalizedImpulse = NormalizeByMass(impulseMagnitude, ObjectMass, .75f);
	
	if (ShouldPlayImpact(normalizedImpulse))
	{
		CurrentImpactCooldown = 0.f;

		float impactRTPCValue = FMath::GetMappedRangeValueClamped(
			FVector2D(50.f, 2000.f),
			FVector2D(0.f, 100.f),
			normalizedImpulse
		);
		
		if (!FMath::IsNearlyEqual(PreviousImpactRTPCValue, impactRTPCValue, UPhysicsAudioSettings::GetRTPCTolerance())) // Set RTPC only when it changed
		{
			PreviousImpactRTPCValue = impactRTPCValue;
			SetRTPCValue(RTPCAssets->ImpactRTPC, FMath::Clamp(PreviousImpactRTPCValue, 0.f, 100.f), 0, FString());
		}
		PostAkEvent(GetAkEventByType(EPAEventType::Impact), 0, FOnAkPostEventCallback());
		//FLinearColor debugColor = bGrounded ? FLinearColor::White : FLinearColor::Black;
		//UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetComponentLocation(), 100.f,12,debugColor, .5f);
	}
	bGrounded = true;	
}

void UPAPhysicsAudioComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	if (IsValid(ParentComponent))
		ParentComponent->OnComponentHit.AddUniqueDynamic(this, &UPAPhysicsAudioComponent::OnComponentHit);
}

void UPAPhysicsAudioComponent::Deactivate()
{
	PrimaryComponentTick.SetTickFunctionEnable(false);
	if (LIKELY(IsValid(this)))
		StopContinuousSound({ EPAEventType::Slide, EPAEventType::Roll });
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
	{
		HandleStoppingLoops();
		return;
	}		
	// Update physics state
	UpdatePhysicsState(DeltaTime);
	// Update RTPCs for continuous sound changes
	UpdateRTPCValues();
	// Check if we should disable loops
	HandleStoppingLoops();
	HandlePlayingContinuousSounds();
}

void UPAPhysicsAudioComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Deactivate();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}
