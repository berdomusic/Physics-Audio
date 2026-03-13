#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PhysicsAudioSettings.generated.h"

/*namespace PhysicsAudioSettings
{
	static constexpr float PHYSICS_AUDIO_DEACTIVATION_DELAY = 1.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY = 50.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY_SQUARED = FMath::Square(PHYSICS_AUDIO_MIN_VELOCITY);
	static constexpr float PHYSICS_AUDIO_RTPC_TOLERANCE = 1.f;
}*/

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="PA - Physics Audio Settings"))
class PHYSICSAUDIO_API UPhysicsAudioSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPhysicsAudioSettings(const FObjectInitializer& ObjectInitializer);

    /*// UDeveloperSettings interface
    virtual FName GetContainerName() const override { return "Project"; }
    virtual FName GetCategoryName() const override { return "Game"; }
    virtual FName GetSectionName() const override { return "Physics Audio"; }
    // End UDeveloperSettings interface*/

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

    UFUNCTION(BlueprintPure, Category = "Physics Audio")
	static float GetDeactivationDelay();
    UFUNCTION(BlueprintPure, Category = "Physics Audio")
    static float GetMinVelocity();
    UFUNCTION(BlueprintPure, Category = "Physics Audio")
    static float GetMinVelocitySquared();
    UFUNCTION(BlueprintPure, Category = "Physics Audio")
    static float GetRTPCTolerance();

    // Update the cached squared value when min velocity changes
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // Validate settings on load
    virtual void PostInitProperties() override;
};