#include <ph/PhantasyEngineMain.hpp>
#include <ph/game_loop/DefaultGameUpdateable.hpp>
#include <ph/game_loop/GameLoopUpdateable.hpp>

#include <sfz/memory/SmartPointers.hpp>

#include "TestbedLogic.hpp"

static ph::InitOptions createInitOptions()
{
	ph::InitOptions options;

	options.appName = "PhantasyTestbed";

#ifdef __EMSCRIPTEN__
	options.iniLocation = ph::IniLocation::NEXT_TO_EXECUTABLE;
#else
	options.iniLocation = ph::IniLocation::MY_GAMES_DIR;
#endif

	options.createInitialUpdateable = []() -> sfz::UniquePtr<ph::GameLoopUpdateable> {
		Allocator* allocator = sfz::getDefaultAllocator();
		return ph::createDefaultGameUpdateable(allocator, createTestbedLogic(allocator));
	};

#ifdef __EMSCRIPTEN__
	options.rendererName = "Renderer-CompatibleGL";
#else
	options.rendererName = "Renderer-CompatibleGL";
#endif

	return options;
}

PHANTASY_ENGINE_MAIN(createInitOptions);
