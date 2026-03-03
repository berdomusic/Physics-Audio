// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PAPhysicsAudioComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/PhysicsAudioStructs.h"

#include "PAPhysicsAudioSubsystem.generated.h"

/**
 * 
 */
struct FPAPhysicsAudioQueueInfo
{
	UPrimitiveComponent* TargetComponent = nullptr;
	FPAPhysicsActorAudioHandle Handle;

	FPAPhysicsAudioQueueInfo(
		UPrimitiveComponent* InComponent,
		const FPAPhysicsActorAudioHandle& InHandle
	)
		: 
		TargetComponent(InComponent),
		Handle(InHandle)
	{
		check(TargetComponent);
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
	UPrimitiveComponent* TargetComponent = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "PhysicsAudio")
	UPAPhysicsAudioComponent* AudioComponent;
	
	/*bool operator==(FPAActivePhysicsAudioObject Other) const
	{
		return TargetComponent == Other.TargetComponent;
	}*/
	
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
	
	// Call this when you want to begin to use physics audio (for example, after level load)
	UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
	void EnablePhysicsAudio(bool bAsync); 
	UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
	void TryAddPhysicsAudioToPrimitive(UPrimitiveComponent* InComponent, const FPAPhysicsActorAudioHandle& InAudioHandle);
	UFUNCTION(BlueprintCallable, Category = "PhysicsAudio")
	void ReturnPhysicsAudioComponentToPool(UPrimitiveComponent* InComponent);
	void ReturnOrphanedAudioComponentToPool(UPAPhysicsAudioComponent* InComponent);
	UFUNCTION(BlueprintPure)
	TArray<FVector> GetListenersPositions() { return ListenersPositions; }
	
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
	void RunQueue(bool bOneItem);
	void FlushQueue();
	
	bool CheckIfCanAttachAudioComponent(const UPrimitiveComponent* InComponent);
	UPAPhysicsAudioComponent* GetAudioComponentFromPrimitive(const UPrimitiveComponent* InComponent);
	void RemoveAudioObjectFromActivePool(const UPrimitiveComponent* InComponent);
	
	TArray<FVector> ListenersPositions;
	void CacheListenersPositions();
	void UpdateDistanceToListeners();
	
	bool CanAddComponentToPool() const;
	FTSTicker::FDelegateHandle TickHandle;
	bool Tick(float DeltaTime);
};
