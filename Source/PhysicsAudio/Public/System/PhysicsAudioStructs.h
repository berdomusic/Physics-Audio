#pragma once

#include "CoreMinimal.h"
#include "PhysicsAudioStructs.generated.h"

class UAkAudioEvent;
class UAkRtpc;
class UAkSwitchValue;

UINTERFACE(MinimalAPI, Blueprintable)
class UPhysicsAudioInterface : public UInterface
{
	GENERATED_BODY()
};

class IPhysicsAudioInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "PhysicsAudio")
	void OnHitByProjectile(
		AActor* HitActor,
		UPrimitiveComponent* HitComp,
		AActor* ProjectileActor,
		UPrimitiveComponent* ProjectileComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);
	virtual void OnHitByProjectile_Implementation(AActor* HitActor,
		UPrimitiveComponent* HitComp,
		AActor* ProjectileActor,
		UPrimitiveComponent* ProjectileComp,
		FVector NormalImpulse,
		const FHitResult& Hit
		);
};

UCLASS(BlueprintType)
class PHYSICSAUDIO_API UPAMaterialSwitchMap : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material Switch")
	TMap<TEnumAsByte<EPhysicalSurface>, TSoftObjectPtr<UAkSwitchValue>> MaterialSwitchMap;
};

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
	TSoftObjectPtr<UAkAudioEvent> ImpactSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> SlideSound;
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
