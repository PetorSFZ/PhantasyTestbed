#pragma once

#include <sfz/game_loop/DefaultGameUpdateable.hpp>

using sfz::GameLogic;
using sfz::Allocator;
using sfz::UniquePtr;

// TestbedLogic creation function
// ------------------------------------------------------------------------------------------------

UniquePtr<GameLogic> createTestbedLogic(Allocator* allocator) noexcept;
