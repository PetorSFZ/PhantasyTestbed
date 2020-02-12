#include <skipifzero.hpp>
#include <skipifzero_smart_pointers.hpp>

#include <sfz/Context.hpp>
#include <sfz/PhantasyEngineMain.hpp>
#include <sfz/game_loop/DefaultGameUpdateable.hpp>
#include <sfz/game_loop/GameLoopUpdateable.hpp>

#include "TestbedLogic.hpp"

#if defined(_WIN32) && defined(NDEBUG)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

sfz::InitOptions PhantasyEngineUserMain(int argc, char* argv[])
{
	(void)argc;
	(void)argv;
	sfz::InitOptions options;
	options.appName = "PhantasyTestbed";
#ifdef __EMSCRIPTEN__
	options.iniLocation = sfz::IniLocation::NEXT_TO_EXECUTABLE;
#else
	options.iniLocation = sfz::IniLocation::MY_GAMES_DIR;
#endif
	options.initialGameLogic = createTestbedLogic(sfz::getDefaultAllocator());
	return options;
}
