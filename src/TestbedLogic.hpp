#pragma once

#include <ph/game_loop/DefaultGameUpdateable.hpp>

using ph::GameLogic;
using sfz::Allocator;
using sfz::UniquePtr;

// TestbedLogic creation function
// ------------------------------------------------------------------------------------------------

UniquePtr<GameLogic> createTestbedLogic(Allocator* allocator) noexcept;
