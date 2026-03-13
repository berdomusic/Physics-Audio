// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "PAGameReferencesSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PHYSICSAUDIO_API UPAGameReferencesSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static UPAGameReferencesSubsystem& Get();

	UDataAsset* GetMaterialSwitchDataAsset() const;
	UDataAsset* GetRTPCsDataAsset() const;
	UDataAsset* GetStopContinuousSoundsDataAsset() const;

protected:
	UPROPERTY()
	mutable UDataAsset* MaterialSwitchDataAsset = nullptr;
	UPROPERTY()
	mutable UDataAsset* RTPCsDataAsset = nullptr;
	UPROPERTY()
	mutable UDataAsset* StopContinuousSoundsDataAsset = nullptr;
	
};
