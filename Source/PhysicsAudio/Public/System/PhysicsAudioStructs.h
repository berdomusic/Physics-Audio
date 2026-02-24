#pragma once

#include "CoreMinimal.h"
#include "PhysicsAudioStructs.generated.h"

class UAkAudioEvent;
class UAkRtpc;

UCLASS(BlueprintType)
class PHYSICSAUDIO_API UPAPhysicsRTPCs : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTPC")
	UAkRtpc* VelocityRTPC = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTPC")
	UAkRtpc* MassRTPC = nullptr;
};

USTRUCT(BlueprintType)
struct FPAPhysicsActorAudioHandle : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> CollisionSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> DestructionSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTPC")
	TObjectPtr<UPAPhysicsRTPCs> RTPC_Handle;
};

USTRUCT(BlueprintType)
struct FPAPhysicsActorProperties : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> PhysicsActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D SizeOffsets = FVector2D::UnitVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FDataTableRowHandle PhysicsActorAudioProperties;

};
