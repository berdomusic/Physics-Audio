// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AkSwitchValue.h"
#include "Engine/StreamableManager.h"
#include "System/PhysicsAudioStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PAFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PHYSICSAUDIO_API UPAFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static UAkSwitchValue* GetAkSwitchFromSurface(const TEnumAsByte<EPhysicalSurface> InSurface);
	static UAkAudioEvent* GetStopContinousSoundEvent(EPAEventType InEventType);
	UFUNCTION(BlueprintPure)
	static bool IsAudioHandleNotEmpty(const FPAPhysicsActorAudioProperties& InHandle);
	static bool ResolveAudioHandle(const FDataTableRowHandle& InRowHandle, FPAPhysicsActorAudioProperties& OutHandle);
	UFUNCTION(BlueprintPure)
	static TArray<TSoftObjectPtr<UAkAudioEvent>> GetAkEventsSoftFromHandle(const FPAPhysicsActorAudioProperties& InHandle);
	static void LoadEventsFromHandle(const FPAPhysicsActorAudioProperties& InHandle, const FStreamableDelegate& OnLoaded = FStreamableDelegate());
	
	UFUNCTION(BlueprintPure, meta = (WorldContext = "InWorldContext", CompactNodeTitle = "GetPickupModifier"))
	static float GetCurrentPickupLengthModifier(const UObject* InWorldContext);
};
