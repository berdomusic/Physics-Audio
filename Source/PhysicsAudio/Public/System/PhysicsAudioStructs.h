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

UINTERFACE(MinimalAPI, BlueprintType)
class ULookAtInterface : public UInterface
{
	GENERATED_BODY()
};

class ILookAtInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "LookAt")
	void OnLookAtStarted();
	virtual void OnLookAtStarted_Implementation()	{	}
	UFUNCTION(BlueprintNativeEvent, Category = "LookAt")
	void OnLookAtFinished();
	virtual void OnLookAtFinished_Implementation()	{	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LookAt")
	void OnPickup(AActor* InInstigator);
	//virtual void OnPickup_Implementation(AController* InteractionInstigator);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LookAt")
	void OnDrop(AActor* InInstigator);
	//virtual void OnDrop_Implementation(AController* InteractionInstigator);
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

UENUM()
enum class EPAEventType : uint8
{
	Impact,
	Slide,
	Roll,
	Projectile,
	Destruction,
	Count
};

USTRUCT(BlueprintType)
struct FPAPhysicsAudioEvent
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TSoftObjectPtr<UAkAudioEvent> AkEventSoft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	float CooldownThreshold = .1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	bool bLoadSynchronously;
};

USTRUCT(BlueprintType)
struct FPAPhysicsActorAudioProperties : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mass")
	float ObjectMassOverride = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FPAPhysicsAudioEvent ImpactSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slide")
	FPAPhysicsAudioEvent SlideSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	FPAPhysicsAudioEvent RollSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FPAPhysicsAudioEvent ProjectileSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destruction")
	FPAPhysicsAudioEvent DestructionSound;
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
	bool bPickable = true;	
};

