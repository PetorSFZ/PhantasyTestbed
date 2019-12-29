#include <skipifzero.hpp>
#include <skipifzero_smart_pointers.hpp>

#include <sfz/Context.hpp>
#include <sfz/PhantasyEngineMain.hpp>
#include <sfz/game_loop/DefaultGameUpdateable.hpp>
#include <sfz/game_loop/GameLoopUpdateable.hpp>

#include "TestbedLogic.hpp"

static sfz::InitOptions createInitOptions()
{
	sfz::InitOptions options;

	options.appName = "PhantasyTestbed";

#ifdef __EMSCRIPTEN__
	options.iniLocation = sfz::IniLocation::NEXT_TO_EXECUTABLE;
#else
	options.iniLocation = sfz::IniLocation::MY_GAMES_DIR;
#endif

	options.createInitialUpdateable = []() -> sfz::UniquePtr<sfz::GameLoopUpdateable> {
		Allocator* allocator = sfz::getDefaultAllocator();
		return sfz::createDefaultGameUpdateable(allocator, createTestbedLogic(allocator));
	};

	return options;
}

PHANTASY_ENGINE_MAIN(createInitOptions);
