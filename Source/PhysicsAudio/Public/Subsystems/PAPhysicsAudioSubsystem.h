#pragma once

#include "CoreMinimal.h"
#include "Components/PAPhysicsAudioComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/PhysicsAudioStructs.h"

#include "PAPhysicsAudioSubsystem.generated.h"

struct FPAPhysicsAudioQueueInfo
{
    TWeakObjectPtr<UPrimitiveComponent> TargetComponent;
    FPAPhysicsActorAudioProperties Handle;

    FPAPhysicsAudioQueueInfo(UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioProperties& InHandle)
        : TargetComponent(InComponent), Handle(InHandle)
    {
        check(TargetComponent.IsValid());
    }

    bool operator==(const FPAPhysicsAudioQueueInfo& other) const
    {
        return TargetComponent == other.TargetComponent;
    }
};

USTRUCT(BlueprintType)
struct FPAActivePhysicsAudioObject
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "PhysicsAudio")
    TWeakObjectPtr<UPrimitiveComponent> TargetComponent;
    UPROPERTY(BlueprintReadOnly, Category = "PhysicsAudio")
    TWeakObjectPtr<UPAPhysicsAudioComponent> AudioComponent;

    bool operator==(const FPAActivePhysicsAudioObject& Other) const
    {
        return TargetComponent == Other.TargetComponent;
    }
};

UCLASS()
class PHYSICSAUDIO_API UPAPhysicsAudioSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    static UPAPhysicsAudioSubsystem* Get(const UWorld* World);
    
    UFUNCTION(BlueprintPure, Category = "PhysicsAudio")
    int32 GetCurrentPoolSize() const { return PhysicsAudioPoolSize; }
    UFUNCTION(BlueprintPure, Category = "PhysicsAudio")
    bool IsPhysicsAudioEnabled() const { return bPhysicsAudioEnabled; }
    UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
    void EnablePhysicsAudio(bool bAsync, int32 InPoolSize = 10);
    UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
    void DisablePhysicsAudio();
    UFUNCTION(BlueprintPure, Category = "PhysicsAudio")
    bool CanEnablePhysicsAudio() const;
    UFUNCTION(BlueprintPure, Category = "PhysicsAudio")
    TArray<FVector> GetListenersPositions() const { return ListenersPositions; }
    UFUNCTION(BlueprintPure, Category = "PhysicsAudio")
    void GetPhysicsAudioComponentCount(int32& Available, int32& Active, int32& PendingReturn) const;
   
    void TryAddPhysicsAudioToPrimitive(UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioProperties& InAudioProperties);
    void ReturnPhysicsAudioObjectToPool(UPrimitiveComponent* InComponent, UPAPhysicsAudioComponent* InAudioComponent, bool bWasDestroyed);
private: 
    
    int32 PhysicsAudioPoolSize = 50;
    bool bPhysicsAudioEnabled;

    UPROPERTY()
    TArray<UPAPhysicsAudioComponent*> AvailablePhysicsAudioComponentsPool;
    UPROPERTY()
    TArray<FPAActivePhysicsAudioObject> ActivePhysicsAudioObjects;
    UPROPERTY()
    TArray<UPAPhysicsAudioComponent*> PendingReturnPool;    
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    
    void PopulatePoolAsync();
    void TryAddComponentToPool(UPAPhysicsAudioComponent* InComponent);
    UPAPhysicsAudioComponent* TryGetAudioComponentFromPool();

    TArray<FPAPhysicsAudioQueueInfo> PhysicsAudioQueue;    

    void RunQueue(bool bOneItem);
    void FlushQueue();
    
    void ProcessQueueItem(const FPAPhysicsAudioQueueInfo& QueueItem);
    void ProcessPendingReturn(bool bOneItem);
    void ProcessAudioQueue(bool bOneItem);

    bool CheckIfCanAttachAudioComponent(const UPrimitiveComponent* InComponent) const;
    void AddAudioObjectToReturnQueue(const FPAActivePhysicsAudioObject& InAudioObject);

    TArray<FVector> ListenersPositions;
    void CacheListenersPositions();
    void UpdateDistanceToListeners();

    bool CanAddComponentToPool() const;
    
    FPAActivePhysicsAudioObject GetActiveAudioObject(bool& bSuccess, const UPrimitiveComponent* InComponent,
        const UPAPhysicsAudioComponent* InAudioComponent);

    FTSTicker::FDelegateHandle TickHandle;
    bool Tick(float DeltaTime);
};
