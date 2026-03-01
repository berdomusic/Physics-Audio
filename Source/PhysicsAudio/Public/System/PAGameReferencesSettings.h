// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PAGameReferencesSettings.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FPAGameReferenceDataAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UDataAsset> DataAsset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bKeepLoaded = true;
};

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "PA - Game References"))
class PHYSICSAUDIO_API UPAGameReferencesSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	static const UPAGameReferencesSettings& Get();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Game References Settings"))
	static const UPAGameReferencesSettings* K2_Get();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, config)
	FPAGameReferenceDataAsset MaterialSwitchesSoft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, config)
	FPAGameReferenceDataAsset RTPCs_Soft;

};
