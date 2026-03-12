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
    if (!CanEnablePhysicsAudio())
        return;
    PhysicsAudioPoolSize = InPoolSize;
    bPhysicsAudioEnabled = true;

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

void UPAPhysicsAudioSubsystem::DisablePhysicsAudio()
{
    if (!bPhysicsAudioEnabled)
        return;
    FlushQueue();
    bPhysicsAudioEnabled = false;
    
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();
    
    for (const FPAActivePhysicsAudioObject& ActiveObject : ActivePhysicsAudioObjects)
        if (ActiveObject.AudioComponent.IsValid())
        {
            ActiveObject.AudioComponent.Get()->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
            ActiveObject.AudioComponent.Get()->DeactivatePhysicsAudioComponent();
            ActiveObject.AudioComponent.Get()->DestroyComponent();
        }    
    ActivePhysicsAudioObjects.Empty();
    
    for (UPAPhysicsAudioComponent* AudioComponent : AvailablePhysicsAudioComponentsPool)
        if (IsValid(AudioComponent))
            AudioComponent->DestroyComponent();
    AvailablePhysicsAudioComponentsPool.Empty();
}

bool UPAPhysicsAudioSubsystem::CanEnablePhysicsAudio() const
{
    return !bPhysicsAudioEnabled 
    && AvailablePhysicsAudioComponentsPool.IsEmpty()
    && ActivePhysicsAudioObjects.IsEmpty();
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

void UPAPhysicsAudioSubsystem::GetPhysicsAudioComponentCount(int32& Available, int32& Active, int32& PendingReturn) const
{
    Available = AvailablePhysicsAudioComponentsPool.Num();
    Active = ActivePhysicsAudioObjects.Num();
    PendingReturn = PendingReturnPool.Num();
}

void UPAPhysicsAudioSubsystem::TryAddPhysicsAudioToPrimitive(UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioProperties& InAudioProperties)
{
    if (!IsValid(InComponent))
        return;
    if (!CheckIfCanAttachAudioComponent(InComponent))
        return;
    PhysicsAudioQueue.AddUnique(FPAPhysicsAudioQueueInfo(InComponent, InAudioProperties));        
}

void UPAPhysicsAudioSubsystem::ReturnPhysicsAudioObjectToPool(UPrimitiveComponent* InComponent, UPAPhysicsAudioComponent* InAudioComponent, bool bWasDestroyed)
{
    bool bSuccess = false;
    FPAActivePhysicsAudioObject audioObject = GetActiveAudioObject(bSuccess, InComponent, InAudioComponent);
    if (bSuccess)
    {
        UPAPhysicsAudioComponent* audioComp = audioObject.AudioComponent.Get();
        if (IsValid(audioComp))
        {
            if (bWasDestroyed)
                audioComp->OnParentDestroyed();        
            audioComp->DeactivatePhysicsAudioComponent();
            audioComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);    
        } 
        AddAudioObjectToReturnQueue(audioObject);
    }        
}

void UPAPhysicsAudioSubsystem::RunQueue(bool bOneItem)
{
    ProcessPendingReturn(bOneItem);
    if (bPhysicsAudioEnabled)
        ProcessAudioQueue(bOneItem);
}

void UPAPhysicsAudioSubsystem::FlushQueue()
{
    RunQueue(false);
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
    if (QueueItem.TargetComponent.Get()->IsSimulatingPhysics())
        componentToProcess->OnAttachedToSimulatingComponent(QueueItem.TargetComponent.Get(), QueueItem.Handle);
    else
        componentToProcess->OnAttachedToNonSimulatingComponent(QueueItem.TargetComponent.Get(), QueueItem.Handle);
    ActivePhysicsAudioObjects.Emplace(FPAActivePhysicsAudioObject{ QueueItem.TargetComponent, componentToProcess });
}

void UPAPhysicsAudioSubsystem::ProcessPendingReturn(bool bOneItem)
{
    while (!PendingReturnPool.IsEmpty())
    {
        UPAPhysicsAudioComponent* object = PendingReturnPool[0];
        PendingReturnPool.RemoveAt(0);
        if (IsValid(object))
        {
            TryAddComponentToPool(object);
            if (bOneItem)
                return;
        }
    }
}

void UPAPhysicsAudioSubsystem::ProcessAudioQueue(bool bOneItem)
{
    while (!PhysicsAudioQueue.IsEmpty())
    {
        const FPAPhysicsAudioQueueInfo QueueItem = PhysicsAudioQueue[0];
        PhysicsAudioQueue.RemoveAt(0);
        ProcessQueueItem(QueueItem);
        if (bOneItem)
            return;
    }
}

bool UPAPhysicsAudioSubsystem::CheckIfCanAttachAudioComponent(const UPrimitiveComponent* InComponent) const
{
    if (!IsValid(InComponent) || !bPhysicsAudioEnabled)
        return false;
    for (const FPAActivePhysicsAudioObject& currentObject : ActivePhysicsAudioObjects)
        if (currentObject.TargetComponent.IsValid() && currentObject.TargetComponent.Get() == InComponent)
            return false;
    return true;
}

void UPAPhysicsAudioSubsystem::AddAudioObjectToReturnQueue(const FPAActivePhysicsAudioObject& InAudioObject)
{
    for (int i = ActivePhysicsAudioObjects.Num() - 1; i >= 0; --i)
        if (ActivePhysicsAudioObjects[i] == InAudioObject)
        {
            ActivePhysicsAudioObjects.RemoveAt(i);
            if (InAudioObject.AudioComponent.IsValid())
                PendingReturnPool.AddUnique(InAudioObject.AudioComponent.Get());         
            break;
        }
}

void UPAPhysicsAudioSubsystem::CacheListenersPositions()
{
    ListenersPositions.Empty();
    if (FAkAudioDevice* AudioDevice = FAkAudioDevice::Get())
    {
        UAkComponentSet& defaultListeners = AudioDevice->GetDefaultListeners();
        for (const TWeakObjectPtr<UAkComponent>& weakListener : defaultListeners)
            if (UAkComponent* Listener = weakListener.Get())
                ListenersPositions.AddUnique(Listener->GetComponentLocation());
    }
}

void UPAPhysicsAudioSubsystem::UpdateDistanceToListeners()
{
    CacheListenersPositions();
    if (!ListenersPositions.IsEmpty() && !ActivePhysicsAudioObjects.IsEmpty())
    {
        for (FPAActivePhysicsAudioObject& object : ActivePhysicsAudioObjects)
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
        bPhysicsAudioEnabled
            && ActivePhysicsAudioObjects.Num() 
            + AvailablePhysicsAudioComponentsPool.Num()
            + PendingReturnPool.Num() 
                < PhysicsAudioPoolSize;
}

FPAActivePhysicsAudioObject UPAPhysicsAudioSubsystem::GetActiveAudioObject(bool& bSuccess, const UPrimitiveComponent* InComponent,
    const UPAPhysicsAudioComponent* InAudioComponent)
{
    bSuccess = false;
    if (IsValid(InComponent))
    {
        for (int i = ActivePhysicsAudioObjects.Num() - 1; i >= 0; --i)
            if (ActivePhysicsAudioObjects[i].TargetComponent.IsValid())
                if (ActivePhysicsAudioObjects[i].TargetComponent.Get() == InComponent)
                {
                    bSuccess = true;
                    return ActivePhysicsAudioObjects[i];
                }
           
    }
    else if (IsValid(InAudioComponent))
    {
        for (int i = ActivePhysicsAudioObjects.Num() - 1; i >= 0; --i)
            if (ActivePhysicsAudioObjects[i].AudioComponent.IsValid())
                if (ActivePhysicsAudioObjects[i].AudioComponent.Get() == InAudioComponent)
                {
                    bSuccess = true;
                    return ActivePhysicsAudioObjects[i];
                }
    }    
    return FPAActivePhysicsAudioObject();
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
