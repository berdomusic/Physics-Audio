// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AkSwitchValue.h"
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
};
