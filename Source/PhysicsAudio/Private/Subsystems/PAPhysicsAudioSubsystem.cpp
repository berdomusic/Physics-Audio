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
			AvailablePhysicsAudioComponentsPool.Add(component);
		}
		
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPAPhysicsAudioSubsystem::Tick)
	);		
	}
}

void UPAPhysicsAudioSubsystem::TryAddComponentToPool(UPAPhysicsAudioComponent* InComponent)
{
	if (AvailablePhysicsAudioComponentsPool.Num() < PhysicsAudioSettings::PHYSICS_AUDIO_POOL_SIZE)
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
	else //means that we don't need that component anymore
	{
		InComponent->DestroyComponent();
	}
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
				newComponent->OnAttachedToPhysicsComponent(QueueItem.TargetComponent, QueueItem.Handle);
				ActivePhysicsAudioObjectsPool.AddUnique(FPAActivePhysicsAudioObject {QueueItem.TargetComponent, newComponent});
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

UPAPhysicsAudioComponent* UPAPhysicsAudioSubsystem::GetAudioComponentFromPrimitive(UPrimitiveComponent* InComponent)
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
			ActivePhysicsAudioObjectsPool.RemoveAt(i);
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
	if (AvailablePhysicsAudioComponentsPool.Num() < PhysicsAudioSettings::PHYSICS_AUDIO_POOL_SIZE)
	{
		TryAddComponentToPool(nullptr);
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UPAPhysicsAudioSubsystem::PopulatePoolAsync)
		);
	}
	else
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPAPhysicsAudioSubsystem::Tick)
	);		
	}
}

bool UPAPhysicsAudioSubsystem::Tick(float DeltaTime)
{
	if (TickCounter < 1)
	{
		++TickCounter;
		return true;
	}

	TickCounter = 0;
	RunQueue(true);
	return true;
}
