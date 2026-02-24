// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsAudioGameMode.h"
#include "PhysicsAudioCharacter.h"
#include "UObject/ConstructorHelpers.h"

APhysicsAudioGameMode::APhysicsAudioGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
