#pragma once

#include "CoreMinimal.h"

namespace PhysicsAudioSettings
{
	static constexpr float PHYSICS_AUDIO_DEACTIVATION_DELAY = 2.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY = 50.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY_SQUARED = FMath::Square(PHYSICS_AUDIO_MIN_VELOCITY);
	static constexpr float PHYSICS_AUDIO_COOLDOWN_TIME = .1f;
	static constexpr float PHYSICS_AUDIO_RTPC_TOLERANCE = 1.f;
}