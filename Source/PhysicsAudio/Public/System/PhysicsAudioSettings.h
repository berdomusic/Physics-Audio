#pragma once

#include "CoreMinimal.h"

namespace PhysicsAudioSettings
{
	static constexpr int8 PHYSICS_AUDIO_POOL_SIZE = 50;
	static constexpr float PHYSICS_AUDIO_DEACTIVATION_DELAY = 2.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY = 50.f;
	static constexpr float PHYSICS_AUDIO_MIN_VELOCITY_SQUARED = FMath::Square(PHYSICS_AUDIO_MIN_VELOCITY);
	static constexpr float PHYSICS_AUDIO_COOLDOWN_TIME = .1f;
}