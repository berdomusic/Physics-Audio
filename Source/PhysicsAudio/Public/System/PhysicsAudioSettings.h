#pragma once

#include "CoreMinimal.h"

namespace PhysicsAudioSettings
{
	static constexpr float PHYSICS_AUDIO_DEACTIVATION_DELAY = 1.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY = 100.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY_SQUARED = FMath::Square(PHYSICS_AUDIO_MIN_VELOCITY);
	static constexpr float PHYSICS_AUDIO_RTPC_TOLERANCE = 1.f;
}