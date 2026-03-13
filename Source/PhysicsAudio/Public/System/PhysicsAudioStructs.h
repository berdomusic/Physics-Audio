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
	void OnPhysicsActorHit(FVector NormalImpulse, const FHitResult& Hit);
};

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
	UFUNCTION(BlueprintNativeEvent, Category = "LookAt")
	void OnLookAtFinished();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LookAt")
	void OnPickup(AActor* InInstigator);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LookAt")
	void OnDrop(AActor* InInstigator);
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

UCLASS(BlueprintType)
class PHYSICSAUDIO_API UPAStopContinousSoundEvents : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ContinousSound")
	UAkAudioEvent* StopRoll = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ContinousSound")
	UAkAudioEvent* StopSlide = nullptr;
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0"))
	float CooldownThreshold = .1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	bool bLoadSynchronous;
};

USTRUCT(BlueprintType)
struct FPAPhysicsActorAudioProperties : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mass", meta = (ClampMin = "0"))
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

