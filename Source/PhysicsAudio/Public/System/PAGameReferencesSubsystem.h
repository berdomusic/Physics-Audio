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
	
public:
	static UPAGameReferencesSubsystem& Get();

	UDataAsset* GetMaterialSwitchDataAsset() const;

protected:

	UPROPERTY()
	mutable UDataAsset* MaterialSwitchDataAsset = nullptr;
	
};
