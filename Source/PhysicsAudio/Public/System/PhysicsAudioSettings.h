#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PhysicsAudioSettings.generated.h"


UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="PA - Physics Audio Settings"))
class PHYSICSAUDIO_API UPhysicsAudioSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPhysicsAudioSettings(const FObjectInitializer& ObjectInitializer);

    /** Delay before deactivating physics audio attempts */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Physics Audio", meta = (ClampMin = "0.0", Units = "s"))
    float PhysicsAudioDeactivationDelay = 1.f;

    /** Minimum velocity required to trigger physics audio */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Physics Audio", meta = (ClampMin = "0.0", Units = "cm/s"))
    float PhysicsAudioMinVelocity = 50.f;

    /** Square of minimum velocity for optimized comparisons */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Physics Audio", meta = (DisplayName = "Min Velocity Squared"))
    float PhysicsAudioMinVelocitySquared = FMath::Square(PhysicsAudioMinVelocity);

    /** Tolerance for RTPC value updates to prevent unnecessary updates */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Physics Audio", meta = (ClampMin = "0.0"))
    float PhysicsAudioRTPCTolerance = 5.f;
	
	/** Global Time Dilation affects Wwise Time Stretch effect */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Physics Audio")
	bool TimeDilationAffectsSound = false;

    UFUNCTION(BlueprintPure, Category = "Physics Audio")
	static float GetDeactivationDelay();
    UFUNCTION(BlueprintPure, Category = "Physics Audio")
    static float GetMinVelocity();
    UFUNCTION(BlueprintPure, Category = "Physics Audio")
    static float GetMinVelocitySquared();
    UFUNCTION(BlueprintPure, Category = "Physics Audio")
    static float GetRTPCTolerance();
	UFUNCTION(BlueprintPure, Category = "Physics Audio")
	static bool GetTimeDilationAffectsSound();

    // Update the cached squared value when min velocity changes
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // Validate settings on load
    virtual void PostInitProperties() override;
};