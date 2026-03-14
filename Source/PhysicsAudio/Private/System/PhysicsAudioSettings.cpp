#include "System/PhysicsAudioSettings.h"

UPhysicsAudioSettings::UPhysicsAudioSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PhysicsAudioMinVelocitySquared = FMath::Square(PhysicsAudioMinVelocity);
}

void UPhysicsAudioSettings::PostInitProperties()
{
	Super::PostInitProperties();
	PhysicsAudioMinVelocitySquared = FMath::Square(PhysicsAudioMinVelocity);
}

float UPhysicsAudioSettings::GetDeactivationDelay()
{
	const UPhysicsAudioSettings* Settings = GetDefault<UPhysicsAudioSettings>();
	return Settings ? Settings->PhysicsAudioDeactivationDelay : 1.0f;
}

float UPhysicsAudioSettings::GetMinVelocity()
{
	const UPhysicsAudioSettings* Settings = GetDefault<UPhysicsAudioSettings>();
	return Settings ? Settings->PhysicsAudioMinVelocity : 100.0f;
}

float UPhysicsAudioSettings::GetMinVelocitySquared()
{
	const UPhysicsAudioSettings* Settings = GetDefault<UPhysicsAudioSettings>();
	return Settings ? Settings->PhysicsAudioMinVelocitySquared : FMath::Square(100.0f);
}

float UPhysicsAudioSettings::GetRTPCTolerance()
{
	const UPhysicsAudioSettings* Settings = GetDefault<UPhysicsAudioSettings>();
	return Settings ? Settings->PhysicsAudioRTPCTolerance : 1.0f;
}

bool UPhysicsAudioSettings::GetTimeDilationAffectsSound()
{
	const UPhysicsAudioSettings* Settings = GetDefault<UPhysicsAudioSettings>();
	return Settings ? Settings->TimeDilationAffectsSound : false;
}

#if WITH_EDITOR
void UPhysicsAudioSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && 
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhysicsAudioSettings, PhysicsAudioMinVelocity))
	{
		PhysicsAudioMinVelocitySquared = FMath::Square(PhysicsAudioMinVelocity);
	}
}
#endif