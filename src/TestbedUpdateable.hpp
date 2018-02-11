#pragma once

#include <sfz/containers/DynArray.hpp>
#include <sfz/util/FrametimeStats.hpp>
#include <ph/config/GlobalConfig.hpp>
#include <ph/game_loop/GameLoopUpdateable.hpp>
#include <ph/rendering/CameraData.h>
#include <ph/rendering/ImguiRenderingData.h>

#include "SponzaLoader.hpp"

#include <SDL.h>

using ph::GameLoopUpdateable;
using ph::Renderer;
using ph::UpdateOp;
using ph::UpdateInfo;
using ph::UserInput;
using sfz::DynArray;
using sfz::FrametimeStats;
using ph::sdl::ButtonState;
using ph::sdl::Mouse;
using sfz::vec3;

// TestbedUpdateable
// ------------------------------------------------------------------------------------------------

class TestbedUpdateable final : public GameLoopUpdateable {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	TestbedUpdateable() = default;
	TestbedUpdateable(const TestbedUpdateable&) = delete;
	TestbedUpdateable& operator= (const TestbedUpdateable&) = delete;

	virtual ~TestbedUpdateable() noexcept;

	// Overriden methods from GameLoopUpdateable
	// --------------------------------------------------------------------------------------------

	void initialize(Renderer& renderer) override final;
	UpdateOp processInput(
		const UserInput& input,
		const UpdateInfo& updateInfo,
		Renderer& renderer) override final;
	UpdateOp updateTick(const UpdateInfo& updateInfo) override final;
	void render(const UpdateInfo& updateInfo, Renderer& renderer) override final;

private:
	// Private structs
	// --------------------------------------------------------------------------------------------

	struct EmulatedGameController {
		ph::sdl::GameControllerState state;

		ButtonState leftStickUp = ButtonState::NOT_PRESSED;
		ButtonState leftStickDown = ButtonState::NOT_PRESSED;
		ButtonState leftStickLeft = ButtonState::NOT_PRESSED;
		ButtonState leftStickRight = ButtonState::NOT_PRESSED;

		ButtonState shiftPressed = ButtonState::NOT_PRESSED;

		ButtonState rightStickUp = ButtonState::NOT_PRESSED;
		ButtonState rightStickDown = ButtonState::NOT_PRESSED;
		ButtonState rightStickLeft = ButtonState::NOT_PRESSED;
		ButtonState rightStickRight = ButtonState::NOT_PRESSED;
	};

	// Private methods
	// --------------------------------------------------------------------------------------------

	void setDir(vec3 direction, vec3 up) noexcept;

	void updateEmulatedController(const DynArray<SDL_Event>& events, const Mouse& rawMouse) noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	bool mInitialized = false;
	ph::CameraData mCam;
	EmulatedGameController mEmulatedController;
	ph::GameControllerState mCtrl;
	DynArray<ph::SphereLight> mDynamicSphereLights;
	ph::Level mLevel;
	DynArray<ph::RenderEntity> mEntities;

	FrametimeStats mStats = FrametimeStats(480);
	int mStatsWarmup = 0;

	// Imgui
	DynArray<ph::ImguiVertex> mImguiVertices;
	DynArray<uint32_t> mImguiIndices;
	DynArray<ph::ImguiCommand> mImguiCommands;

	// Global Config
	DynArray<sfz::StackString32> mCfgSections;
	DynArray<ph::Setting*> mCfgSectionSettings;
};
