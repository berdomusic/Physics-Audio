#pragma once

#include "CoreMinimal.h"
#include "PhysicsAudioStructs.generated.h"

USTRUCT(BlueprintType)
struct FPAPhysicsActorAudioHandle : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	USoundCue* CollisionSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Params")
	float MinVelocityToSpawnCollisionSound = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Params")
	float MinVelocityDeltaToSpawnSound = 50.f;
};

USTRUCT(BlueprintType)
struct FPAPhysicsActorProperties : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> PhysicsActorClass;	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FDataTableRowHandle PhysicsActorAudioProperties;

};
