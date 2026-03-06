#include "Subsystems/PAPhysicsAudioSubsystem.h"
#include "System/PhysicsAudioSettings.h"
#include "AkAudioDevice.h"
#include "TimerManager.h"

UPAPhysicsAudioSubsystem* UPAPhysicsAudioSubsystem::Get(const UWorld* World)
{
    if (IsValid(World))
        return World->GetSubsystem<UPAPhysicsAudioSubsystem>();
    return nullptr;
}

void UPAPhysicsAudioSubsystem::EnablePhysicsAudio(bool bAsync, int32 InPoolSize)
{
    if (!IsValid(GetWorld()))
        return;
    PhysicsAudioPoolSize = InPoolSize;

    if (bAsync)
        PopulatePoolAsync();
    else
    {
        for (int i = 0; i < PhysicsAudioPoolSize; ++i)
        {
            TryAddComponentToPool(nullptr);
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
        {
            if (AvailablePhysicsAudioComponentsPool.Contains(InComponent))
                return;
            newComponent = InComponent;
        }            
        else
        {
            newComponent = NewObject<UPAPhysicsAudioComponent>(this);
            newComponent->RegisterComponentWithWorld(GetWorld());
            //newComponent->RegisterComponent();
        }		
        AvailablePhysicsAudioComponentsPool.Add(newComponent);
    }
    else if (IsValid(InComponent))
        InComponent->DestroyComponent();
}

UPAPhysicsAudioComponent* UPAPhysicsAudioSubsystem::TryGetAudioComponentFromPool()
{
    if (!AvailablePhysicsAudioComponentsPool.IsEmpty())
        if (UPAPhysicsAudioComponent* component = AvailablePhysicsAudioComponentsPool.Pop())
            return component;
    return nullptr;
}

void UPAPhysicsAudioSubsystem::TryAddPhysicsAudioToPrimitive(UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioHandle& InAudioHandle)
{
    if (!IsValid(InComponent))
        return;
    if (!CheckIfCanAttachAudioComponent(InComponent))
        return;
    PhysicsAudioQueue.AddUnique(FPAPhysicsAudioQueueInfo(InComponent, InAudioHandle));        
}

void UPAPhysicsAudioSubsystem::ReturnPhysicsAudioComponentToPool(UPrimitiveComponent* InComponent)
{
    if (!IsValid(InComponent))
        return;
    UPAPhysicsAudioComponent* component = GetAudioComponentFromPrimitive(InComponent);
    if (!IsValid(component))
        return;

    component->OnDetachedFromPhysicsComponent();
    RemoveAudioObjectFromActivePool(InComponent);
    PendingReturnPool.AddUnique(component);
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
    PendingReturnPool.AddUnique(InComponent);
}

void UPAPhysicsAudioSubsystem::ProcessQueueItem(const FPAPhysicsAudioQueueInfo& QueueItem)
{
    if (!QueueItem.TargetComponent.IsValid())
        return;

    UPAPhysicsAudioComponent* componentToProcess = TryGetAudioComponentFromPool();
    if (!IsValid(componentToProcess))
    {
        PhysicsAudioQueue.AddUnique(QueueItem);
        return;
    }

    componentToProcess->AttachToComponent(QueueItem.TargetComponent.Get(), FAttachmentTransformRules::KeepRelativeTransform);
    componentToProcess->OnAttachedToSimulatingComponent(QueueItem.TargetComponent.Get(), QueueItem.Handle);
    ActivePhysicsAudioObjectsPool.Emplace(FPAActivePhysicsAudioObject{ QueueItem.TargetComponent, componentToProcess });
}

void UPAPhysicsAudioSubsystem::RunQueue(bool bOneItem)
{
    if (!PendingReturnPool.IsEmpty())
    {
        for (UPAPhysicsAudioComponent* component : PendingReturnPool)
            TryAddComponentToPool(component);
        PendingReturnPool.Empty();
    }    

    while (!PhysicsAudioQueue.IsEmpty())
    {
        const FPAPhysicsAudioQueueInfo QueueItem = PhysicsAudioQueue[0];
        PhysicsAudioQueue.RemoveAt(0);
        ProcessQueueItem(QueueItem);
        if (bOneItem)
            return;
    }
}

void UPAPhysicsAudioSubsystem::FlushQueue()
{
    RunQueue(false);
}

bool UPAPhysicsAudioSubsystem::CheckIfCanAttachAudioComponent(const UPrimitiveComponent* InComponent) const
{
    if (!IsValid(InComponent))
        return false;
    for (const FPAActivePhysicsAudioObject& currentObject : ActivePhysicsAudioObjectsPool)
        if (currentObject.TargetComponent.IsValid() && currentObject.TargetComponent.Get() == InComponent)
            return false;
    return true;
}

UPAPhysicsAudioComponent* UPAPhysicsAudioSubsystem::GetAudioComponentFromPrimitive(const UPrimitiveComponent* InComponent)
{
    for (const FPAActivePhysicsAudioObject& currentObject : ActivePhysicsAudioObjectsPool)
    {
        if (currentObject.TargetComponent.IsValid() && currentObject.TargetComponent.Get() == InComponent)
            return currentObject.AudioComponent.Get();
    }
    return nullptr;
}

void UPAPhysicsAudioSubsystem::RemoveAudioObjectFromActivePool(const UPrimitiveComponent* InComponent)
{
    for (int i = ActivePhysicsAudioObjectsPool.Num() - 1; i >= 0; --i)
        if (ActivePhysicsAudioObjectsPool[i].TargetComponent.Get() == InComponent)
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
                ListenersPositions.AddUnique(Listener->GetComponentLocation());
    }
}

void UPAPhysicsAudioSubsystem::UpdateDistanceToListeners()
{
    CacheListenersPositions();
    if (!ListenersPositions.IsEmpty() && !ActivePhysicsAudioObjectsPool.IsEmpty())
    {
        for (FPAActivePhysicsAudioObject& object : ActivePhysicsAudioObjectsPool)
        {
            if (!object.AudioComponent.IsValid())
                continue;

            const FVector componentLocation = object.AudioComponent->GetComponentLocation();
            float distanceToClosestListenerSquared = TNumericLimits<float>::Max();

            for (const FVector& ListenerPosition : ListenersPositions)
            {
                const float distSq = FVector::DistSquared(componentLocation, ListenerPosition);
                if (distSq < distanceToClosestListenerSquared)
                    distanceToClosestListenerSquared = distSq;
            }
            object.AudioComponent->SetSquaredDistanceToClosestListener(distanceToClosestListenerSquared);
        }
    }
}

bool UPAPhysicsAudioSubsystem::CanAddComponentToPool() const
{
    return 
        ActivePhysicsAudioObjectsPool.Num() 
        + AvailablePhysicsAudioComponentsPool.Num()
        + PendingReturnPool.Num() 
            < PhysicsAudioPoolSize;
}

void UPAPhysicsAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    AvailablePhysicsAudioComponentsPool.Reserve(PhysicsAudioPoolSize);
}

void UPAPhysicsAudioSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    Super::Deinitialize();
}

void UPAPhysicsAudioSubsystem::PopulatePoolAsync()
{
    if (!IsValid(GetWorld()))
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
    RunQueue(true);
    UpdateDistanceToListeners();
    return true;
}
