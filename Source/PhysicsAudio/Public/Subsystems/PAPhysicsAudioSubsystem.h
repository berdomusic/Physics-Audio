#pragma once

#include "CoreMinimal.h"
#include "Components/PAPhysicsAudioComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/PhysicsAudioStructs.h"

#include "PAPhysicsAudioSubsystem.generated.h"

struct FPAPhysicsAudioQueueInfo
{
    TWeakObjectPtr<UPrimitiveComponent> TargetComponent;
    FPAPhysicsActorAudioHandle Handle;

    FPAPhysicsAudioQueueInfo(UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioHandle& InHandle)
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

    UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
    void EnablePhysicsAudio(bool bAsync);

    UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
    void TryAddPhysicsAudioToPrimitive(UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioHandle& InAudioHandle);

    UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
    void ReturnPhysicsAudioComponentToPool(UPrimitiveComponent* InComponent);

    void ReturnOrphanedAudioComponentToPool(UPAPhysicsAudioComponent* InComponent);

    UFUNCTION(BlueprintPure, Category = "PhysicsAudio")
    TArray<FVector> GetListenersPositions() const { return ListenersPositions; }

    UPROPERTY(BlueprintReadOnly, Category = "PhysicsAudio")
    TArray<UPAPhysicsAudioComponent*> AvailablePhysicsAudioComponentsPool;

    UPROPERTY(BlueprintReadOnly, Category = "PhysicsAudio")
    TArray<FPAActivePhysicsAudioObject> ActivePhysicsAudioObjectsPool;

    UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
    void TEST_DestroyComponentsManually();

private:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY()
    AActor* DummyOuter;

    void PopulatePoolAsync();
    void TryAddComponentToPool(UPAPhysicsAudioComponent* InComponent);
    UPAPhysicsAudioComponent* TryGetAudioComponentFromPool();

    TArray<FPAPhysicsAudioQueueInfo> PhysicsAudioQueue;
    UPROPERTY()
    TArray<UPAPhysicsAudioComponent*> PendingReturnPool;

    void RunQueue(bool bOneItem);
    void FlushQueue();

    bool CheckIfCanAttachAudioComponent(const UPrimitiveComponent* InComponent) const;
    UPAPhysicsAudioComponent* GetAudioComponentFromPrimitive(const UPrimitiveComponent* InComponent);
    void RemoveAudioObjectFromActivePool(const UPrimitiveComponent* InComponent);

    TArray<FVector> ListenersPositions;
    void CacheListenersPositions();
    void UpdateDistanceToListeners();

    bool CanAddComponentToPool() const;

    FTSTicker::FDelegateHandle TickHandle;
    bool Tick(float DeltaTime);
};
