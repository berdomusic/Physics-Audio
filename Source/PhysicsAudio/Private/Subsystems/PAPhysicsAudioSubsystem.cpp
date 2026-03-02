// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/PAPhysicsAudioSubsystem.h"
#include "System/PhysicsAudioSettings.h"
#include "Kismet/GameplayStatics.h"

UPAPhysicsAudioSubsystem* UPAPhysicsAudioSubsystem::Get(const UWorld* World)
{
	if (IsValid(World))
		return World->GetSubsystem<UPAPhysicsAudioSubsystem>();
	return nullptr;
}

void UPAPhysicsAudioSubsystem::EnablePhysicsAudio(bool bAsync)
{
	if (!IsValid(GetWorld()))
		return;
	DummyOuter = GetWorld()->SpawnActor<AActor>();
	DummyOuter->SetActorHiddenInGame(true);
	
	if (bAsync)
		PopulatePoolAsync();
	else
	{		
		for (int i = 0; i < PhysicsAudioSettings::PHYSICS_AUDIO_POOL_SIZE; ++i)
		{
			UPAPhysicsAudioComponent* component = NewObject<UPAPhysicsAudioComponent>(DummyOuter);
			component->RegisterComponent();
			TryAddComponentToPool(component);
		}		
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPAPhysicsAudioSubsystem::Tick)
	);		
	}
}

void UPAPhysicsAudioSubsystem::TryAddComponentToPool(UPAPhysicsAudioComponent* InComponent)
{
	if (CanAddComponentToPool())
	{
		UPAPhysicsAudioComponent* newComponent;
		if (IsValid(InComponent))
			newComponent = InComponent;
		else
		{
			newComponent = NewObject<UPAPhysicsAudioComponent>(DummyOuter);
			newComponent->RegisterComponent();
		}		
		AvailablePhysicsAudioComponentsPool.AddUnique(newComponent);
	}
	else //we don't need that component anymore
		InComponent->DestroyComponent();
}

UPAPhysicsAudioComponent* UPAPhysicsAudioSubsystem::TryGetAudioComponentFromPool()
{
	if (!AvailablePhysicsAudioComponentsPool.IsEmpty())
		return AvailablePhysicsAudioComponentsPool.Pop();
	return nullptr;
}

void UPAPhysicsAudioSubsystem::TryAddPhysicsAudioToPrimitive(
	UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioHandle& InAudioHandle)
{
	if (!IsValid(InComponent))
		return;
	if (!CheckIfCanAttachAudioComponent(InComponent))
		return;
	PhysicsAudioQueue.AddUnique(FPAPhysicsAudioQueueInfo{ InComponent, InAudioHandle });
}

void UPAPhysicsAudioSubsystem::ReturnPhysicsAudioComponentToPool(UPrimitiveComponent* InComponent)
{
	if (!IsValid(InComponent))
		return;
	UPAPhysicsAudioComponent* component = GetAudioComponentFromPrimitive(InComponent);
	if (!IsValid(component))
		return;
	component->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	component->OnDetachedFromPhysicsComponent();
	RemoveAudioObjectFromActivePool(InComponent);
	TryAddComponentToPool(component);
}

void UPAPhysicsAudioSubsystem::ReturnOrphanedAudioComponentToPool(UPAPhysicsAudioComponent* InComponent)
{
	if (!IsValid(InComponent))
		return;
	InComponent->OnDetachedFromPhysicsComponent();
	for (int i = 0; i < ActivePhysicsAudioObjectsPool.Num(); ++i)
		if (ActivePhysicsAudioObjectsPool[i].AudioComponent == InComponent)
		{
			ActivePhysicsAudioObjectsPool.RemoveAt(i);
			break;
		}			
	TryAddComponentToPool(InComponent);
}

void UPAPhysicsAudioSubsystem::RunQueue(bool bOneItem)
{
	while (!PhysicsAudioQueue.IsEmpty())
	{
		const FPAPhysicsAudioQueueInfo QueueItem = PhysicsAudioQueue[0];
		PhysicsAudioQueue.RemoveAt(0);

		if (IsValid(QueueItem.TargetComponent) && QueueItem.TargetComponent->GetWorld())
		{
			UPAPhysicsAudioComponent* newComponent = TryGetAudioComponentFromPool();
			if (IsValid(newComponent))
			{	
				newComponent->AttachToComponent(QueueItem.TargetComponent, FAttachmentTransformRules::KeepRelativeTransform);
				newComponent->OnAttachedToPhysicsComponent(QueueItem.TargetComponent, QueueItem.Handle, QueueItem.Handle.ObjectMassOverride);
				ActivePhysicsAudioObjectsPool.Emplace(FPAActivePhysicsAudioObject {QueueItem.TargetComponent, newComponent});
				if (bOneItem)
					return;
			}			
		}
	}
}

void UPAPhysicsAudioSubsystem::FlushQueue()
{
	RunQueue(false);
}

bool UPAPhysicsAudioSubsystem::CheckIfCanAttachAudioComponent(const UPrimitiveComponent* InComponent)
{
	if (!IsValid(InComponent))
		return false;
	for (const FPAActivePhysicsAudioObject& currentObject : ActivePhysicsAudioObjectsPool)
		if (currentObject.TargetComponent == InComponent)
			return false;
	return true;
}

UPAPhysicsAudioComponent* UPAPhysicsAudioSubsystem::GetAudioComponentFromPrimitive(const UPrimitiveComponent* InComponent)
{
	for (const FPAActivePhysicsAudioObject& currentObject : ActivePhysicsAudioObjectsPool)
		if (currentObject.TargetComponent == InComponent)
			return currentObject.AudioComponent;
	return nullptr;
}

void UPAPhysicsAudioSubsystem::RemoveAudioObjectFromActivePool(const UPrimitiveComponent* InComponent)
{
	for (int i = 0; i < ActivePhysicsAudioObjectsPool.Num(); ++i)
		if (ActivePhysicsAudioObjectsPool[i].TargetComponent == InComponent)
		{
			ActivePhysicsAudioObjectsPool.RemoveAt(i);
			break;
		}			
}

void UPAPhysicsAudioSubsystem::CacheListenersPositions()
{
	ListenersPositions.Empty();
	if (FAkAudioDevice* AudioDevice = FAkAudioDevice::Get())
	{
		UAkComponentSet& DefaultListeners = AudioDevice->GetDefaultListeners();
		for (const TWeakObjectPtr<UAkComponent>& WeakListener : DefaultListeners)
			if (UAkComponent* Listener = WeakListener.Get())
			{
				const FVector listenerLocation = Listener->GetComponentLocation();
				ListenersPositions.AddUnique(listenerLocation);
			}
	}
}

bool UPAPhysicsAudioSubsystem::CanAddComponentToPool() const
{
	return ActivePhysicsAudioObjectsPool.Num() + AvailablePhysicsAudioComponentsPool.Num() < PhysicsAudioSettings::PHYSICS_AUDIO_POOL_SIZE;
}

void UPAPhysicsAudioSubsystem::TEST_DestroyComponentsManually()
{
	for (auto& comp : ActivePhysicsAudioObjectsPool)
	{
		RemoveAudioObjectFromActivePool(comp.TargetComponent);
		comp.AudioComponent->DestroyComponent();
	}
		
}

void UPAPhysicsAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AvailablePhysicsAudioComponentsPool.Reserve(PhysicsAudioSettings::PHYSICS_AUDIO_POOL_SIZE);	
}

void UPAPhysicsAudioSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

void UPAPhysicsAudioSubsystem::PopulatePoolAsync()
{
	if (!ensure(GetWorld()))
		return;
	if (CanAddComponentToPool())
	{
		TryAddComponentToPool(nullptr);
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UPAPhysicsAudioSubsystem::PopulatePoolAsync)
		);
	}
	else if (!TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPAPhysicsAudioSubsystem::Tick)
	);		
	}
}

bool UPAPhysicsAudioSubsystem::Tick(float DeltaTime)
{
	if (CanAddComponentToPool())
		PopulatePoolAsync();
	CacheListenersPositions();
	RunQueue(true);
	return true;
}
