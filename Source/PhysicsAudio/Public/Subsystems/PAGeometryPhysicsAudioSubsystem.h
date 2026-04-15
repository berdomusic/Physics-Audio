// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PAGeometryPhysicsAudioSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PHYSICSAUDIO_API UPAGeometryPhysicsAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	
private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
};
