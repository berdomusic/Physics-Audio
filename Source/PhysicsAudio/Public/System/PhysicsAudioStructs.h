#pragma once

#include "CoreMinimal.h"
#include "PhysicsAudioStructs.generated.h"

class UAkAudioEvent;
class UAkRtpc;
class UAkSwitchValue;

UINTERFACE(MinimalAPI, Blueprintable)
class UDamageInterface : public UInterface
{
	GENERATED_BODY()
};

class IDamageInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "PhysicsAudio")
	void OnDamageDealt(
		AActor* Dealer,
		const FHitResult& Hit,
		const FVector& InImpulse
	);
};

UINTERFACE(MinimalAPI, Blueprintable)
class UProjectileInterface : public UInterface
{
	GENERATED_BODY()
};

class IProjectileInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "PhysicsAudio")
	void OnHitByProjectile(
		AActor* ProjectileActor,
		const FHitResult& Hit,
		const FVector& InProjectileImpulse
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
	UAkRtpc* ImpactRTPC = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTPC")
	UAkRtpc* SlideRTPC = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTPC")
	UAkRtpc* RollRTPC = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTPC")
	UAkRtpc* MassRTPC = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTPC")
	UAkRtpc* ProjectileRTPC = nullptr;
};

USTRUCT(BlueprintType)
struct FPAPhysicsActorAudioHandle : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mass")
	float ObjectMassOverride = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> ImpactSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> SlideSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> RollSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> ProjectileSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> DestructionSound;
};

USTRUCT(BlueprintType)
struct FPAPhysicsActorProperties : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> PhysicsActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D SizeOffsets = FVector2D::UnitVector;
	/*UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FDataTableRowHandle PhysicsActorAudioProperties;*/
};

