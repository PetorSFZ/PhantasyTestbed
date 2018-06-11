#include "TestbedLogic.hpp"

#include <imgui.h>

#include <sfz/Logging.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/math/Matrix.hpp>

#include <ph/config/GlobalConfig.hpp>
#include <ph/sdl/ButtonState.hpp>

#include "Cube.hpp"
#include "GltfLoader.hpp"
#include "GltfWriter.hpp"

using namespace ph;
using namespace sfz;
using namespace ph::sdl;

// Helper structs
// ------------------------------------------------------------------------------------------------

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

// TestbedLogic class
// ------------------------------------------------------------------------------------------------

class TestbedLogic final : public GameLogic {
public:

	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	TestbedLogic() noexcept = default;
	TestbedLogic(const TestbedLogic&) = delete;
	TestbedLogic& operator= (const TestbedLogic&) = delete;

	// Members
	// --------------------------------------------------------------------------------------------

	EmulatedGameController mEmulatedController;
	ph::GameControllerState mCtrl;

	Setting* mShowImguiDemo = nullptr;

	// Overloaded methods from GameLogic
	// --------------------------------------------------------------------------------------------

	void initialize(UpdateableState& state, Renderer& renderer) override final
	{
		// Add default material
		Material defaultMaterial;
		defaultMaterial.albedo = vec4_u8(255, 0, 0, 255);
		defaultMaterial.roughness = 1.0f;
		state.dynamicAssets.materials.add(defaultMaterial);

		// Load sponza level
		if (!loadAssetsFromGltf("resources/sponza.gltf", state.dynamicAssets)) {
			SFZ_ERROR("PhantasyTesbed", "%s", "Failed to load assets from gltf!");
		}
		// Create RenderEntitites to render
		state.renderEntities.create(state.dynamicAssets.meshes.size());
		for (uint32_t i = 0; i < state.dynamicAssets.meshes.size(); i++) {
			ph::RenderEntity entity;
			entity.meshIndex = i;
			entity.transform = mat34::identity();
			state.renderEntities.add(entity);
		}

		// Uploaded dynamic level assets to renderer
		DynArray<ConstImageView> textureViews;
		for (const auto& texture : state.dynamicAssets.textures) textureViews.add(texture);
		renderer.setTextures(textureViews);
		renderer.setMaterials(state.dynamicAssets.materials);
		DynArray<ConstMeshView> meshViews;
		for (const auto& mesh : state.dynamicAssets.meshes) meshViews.add(mesh);
		renderer.setDynamicMeshes(meshViews);

		// Initialize camera
		state.cam.pos = vec3(3.0f, 3.0f, 3.0f);
		state.cam.dir = sfz::normalize(vec3(-1.0f, -0.25f, -1.0f));
		state.cam.up =  vec3(0.0f, 1.0f, 0.0f);
		state.cam.near = 0.05f;
		state.cam.far = 200.0f;
		state.cam.vertFovDeg = 60.0f;

		// Add dynamic lights
		vec3 lightColors[] = {
			vec3(1.0f, 0.0f, 1.0f),
			vec3(1.0f, 1.0f, 1.0f)
		};
		uint32_t numLights = sizeof(lightColors) / sizeof(vec3);
		for (uint32_t i = 0; i < numLights; i++) {
			ph::SphereLight tmp;

			tmp.pos = vec3(-50.0f + 100.0f * i / (numLights - 1), 5.0f, 0.0f);
			tmp.range = 70.0f;
			tmp.strength = 300.0f * lightColors[i];
			tmp.radius = 0.5f;
			tmp.bitmaskFlags = SPHERE_LIGHT_STATIC_SHADOWS_BIT | SPHERE_LIGHT_DYNAMIC_SHADOWS_BIT;

			state.dynamicSphereLights.add(tmp);
		}

		ph::SphereLight tmpLight;
		tmpLight.pos = vec3(0.0f, 3.0f, 0.0f);
		tmpLight.range = 70.0f;
		tmpLight.radius = 0.5f;
		tmpLight.strength = vec3(150.0f);
		tmpLight.bitmaskFlags = SPHERE_LIGHT_STATIC_SHADOWS_BIT | SPHERE_LIGHT_DYNAMIC_SHADOWS_BIT;
		state.dynamicSphereLights.add(tmpLight);


		GlobalConfig& cfg = GlobalConfig::instance();
		mShowImguiDemo = cfg.sanitizeBool("PhantasyTestbed", "showImguiDemo", true, false);
	}

	virtual ImguiControllers imguiController(const UserInput& input) override final
	{
		ImguiControllers settings;
		settings.useKeyboard = true;
		settings.useMouse = true;
		if (input.controllers.get(0) != nullptr) settings.controllerIndex = 0;
		return settings;
	}

	UpdateOp processInput(
		UpdateableState& state,
		const UserInput& input,
		const UpdateInfo& updateInfo,
		Renderer& renderer) override final
	{
		(void) state;
		(void) updateInfo;
		(void) renderer;

		for (const SDL_Event& event : input.events) {
			if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE) {
				return UpdateOp::QUIT();
			}
		}

		// Update gamecontroller
		updateEmulatedController(input.events, input.rawMouse);
		uint32_t controllerIndex = 0;
		const GameController* controller = input.controllers.get(controllerIndex);
		bool emulatedController = controller == nullptr;
		mCtrl = (emulatedController) ? mEmulatedController.state : controller->state();

		return UpdateOp::NO_OP();
	}

	UpdateOp updateTick(UpdateableState& state, const UpdateInfo& updateInfo) override final
	{
		float delta = updateInfo.tickTimeSeconds;

		float currentSpeed = 10.0f;
		float turningSpeed = 0.8f * PI;

		auto& cam = state.cam;

		// Triggers
		if (mCtrl.leftTrigger > mCtrl.triggerDeadzone) {
			currentSpeed += (mCtrl.leftTrigger * 25.0f);
		}
		if (mCtrl.rightTrigger > mCtrl.triggerDeadzone) {

		}

		// Analogue Sticks
		if (length(mCtrl.rightStick) > mCtrl.stickDeadzone) {
			vec3 right = normalize(cross(cam.dir, cam.up));
			mat3 xTurn = mat3::rotation3(vec3(0.0f, -1.0f, 0.0f), mCtrl.rightStick[0] * turningSpeed * delta);
			mat3 yTurn = mat3::rotation3(right, mCtrl.rightStick[1] * turningSpeed * delta);
			setDir(state, yTurn * xTurn * cam.dir, yTurn * xTurn * cam.up);
		}
		if (length(mCtrl.leftStick) > mCtrl.stickDeadzone) {
			vec3 right = normalize(cross(cam.dir, cam.up));
			cam.pos += ((cam.dir * mCtrl.leftStick[1] + right * mCtrl.leftStick[0]) * currentSpeed * delta);
		}

		// Control Pad
		if (mCtrl.padUp == ButtonState::DOWN) {

		} else if (mCtrl.padDown == ButtonState::DOWN) {

		} else if (mCtrl.padLeft == ButtonState::DOWN) {

		} else if (mCtrl.padRight == ButtonState::DOWN) {

		}

		// Shoulder buttons
		if (mCtrl.leftShoulder == ButtonState::DOWN || mCtrl.leftShoulder == ButtonState::HELD) {
			cam.pos -= vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
		} else if (mCtrl.rightShoulder == ButtonState::DOWN || mCtrl.rightShoulder == ButtonState::HELD) {
			cam.pos += vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
		}

		// Face buttons
		if (mCtrl.y == ButtonState::UP) {
		}
		if (mCtrl.x == ButtonState::UP) {
		}
		if (mCtrl.b == ButtonState::UP) {
		}
		if (mCtrl.a == ButtonState::UP) {
		}

		// Face buttons
		if (mCtrl.y == ButtonState::DOWN) {
		}
		if (mCtrl.x == ButtonState::DOWN) {
		}
		if (mCtrl.b == ButtonState::DOWN) {
		}
		if (mCtrl.a == ButtonState::DOWN) {
		}

		// Menu buttons
		if (mCtrl.back == ButtonState::UP) {
			//return UpdateOp::QUIT();
		}

		setDir(state, cam.dir, vec3(0.0f, 1.0f, 0.0f));

		return UpdateOp::NO_OP();
	}

	void renderCustomImgui() override final
	{
		if (mShowImguiDemo->boolValue()) ImGui::ShowDemoWindow();
	}

	void onConsoleActivated() override final
	{

	}

	void onConsoleDeactivated() override final
	{

	}

	void onQuit(UpdateableState& state) override final
	{
		(void) state;
	}

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	void setDir(UpdateableState& state, vec3 direction, vec3 up) noexcept
	{
		state.cam.dir = normalize(direction);
		state.cam.up = normalize(up - dot(up, state.cam.dir) * state.cam.dir);
		//sfz_assert_debug(approxEqual(dot(mCam.dir, mCam.up), 0.0f));
	}

	void updateEmulatedController(
		const DynArray<SDL_Event>& events,
		const Mouse& rawMouse) noexcept
	{
		EmulatedGameController& ec = mEmulatedController;
		sdl::GameControllerState& c = ec.state;

		// Changes previous DOWN state to HELD state.
		auto changeDownToHeld = [](ButtonState& state) {
			if (state == ButtonState::DOWN) state = ButtonState::HELD;
		};

		changeDownToHeld(c.a);
		changeDownToHeld(c.b);
		changeDownToHeld(c.x);
		changeDownToHeld(c.y);

		changeDownToHeld(c.leftShoulder);
		changeDownToHeld(c.rightShoulder);
		changeDownToHeld(c.leftStickButton);
		changeDownToHeld(c.rightStickButton);

		changeDownToHeld(c.padUp);
		changeDownToHeld(c.padDown);
		changeDownToHeld(c.padLeft);
		changeDownToHeld(c.padRight);

		changeDownToHeld(c.start);
		changeDownToHeld(c.back);
		changeDownToHeld(c.guide);

		changeDownToHeld(ec.leftStickUp);
		changeDownToHeld(ec.leftStickDown);
		changeDownToHeld(ec.leftStickLeft);
		changeDownToHeld(ec.leftStickRight);

		changeDownToHeld(ec.shiftPressed);

		changeDownToHeld(ec.rightStickUp);
		changeDownToHeld(ec.rightStickDown);
		changeDownToHeld(ec.rightStickLeft);
		changeDownToHeld(ec.rightStickRight);


		// Changes previous UP state to NOT_PRESSED state.
		auto changeUpToNotPressed = [](ButtonState& state) {
			if (state == ButtonState::UP) state = ButtonState::NOT_PRESSED;
		};

		changeUpToNotPressed(c.a);
		changeUpToNotPressed(c.b);
		changeUpToNotPressed(c.x);
		changeUpToNotPressed(c.y);

		changeUpToNotPressed(c.leftShoulder);
		changeUpToNotPressed(c.rightShoulder);
		changeUpToNotPressed(c.leftStickButton);
		changeUpToNotPressed(c.rightStickButton);

		changeUpToNotPressed(c.padUp);
		changeUpToNotPressed(c.padDown);
		changeUpToNotPressed(c.padLeft);
		changeUpToNotPressed(c.padRight);

		changeUpToNotPressed(c.start);
		changeUpToNotPressed(c.back);
		changeUpToNotPressed(c.guide);

		changeUpToNotPressed(ec.leftStickUp);
		changeUpToNotPressed(ec.leftStickDown);
		changeUpToNotPressed(ec.leftStickLeft);
		changeUpToNotPressed(ec.leftStickRight);

		changeUpToNotPressed(ec.shiftPressed);

		changeUpToNotPressed(ec.rightStickUp);
		changeUpToNotPressed(ec.rightStickDown);
		changeUpToNotPressed(ec.rightStickLeft);
		changeUpToNotPressed(ec.rightStickRight);


		// Check events from SDL
		for (const SDL_Event& event : events) {
			switch (event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case 'w':
				case 'W':
					ec.leftStickUp = ButtonState::DOWN;
					break;
				case 's':
				case 'S':
					ec.leftStickDown = ButtonState::DOWN;
					break;
				case 'a':
				case 'A':
					ec.leftStickLeft = ButtonState::DOWN;
					break;
				case 'd':
				case 'D':
					ec.leftStickRight = ButtonState::DOWN;
					break;
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
					ec.shiftPressed = ButtonState::DOWN;
					break;
				case SDLK_UP:
					ec.rightStickUp = ButtonState::DOWN;
					break;
				case SDLK_DOWN:
					ec.rightStickDown = ButtonState::DOWN;
					break;
				case SDLK_LEFT:
					ec.rightStickLeft = ButtonState::DOWN;
					break;
				case SDLK_RIGHT:
					ec.rightStickRight = ButtonState::DOWN;
					break;
				case 'q':
				case 'Q':
					c.leftShoulder = ButtonState::DOWN;
					break;
				case 'e':
				case 'E':
					c.rightShoulder = ButtonState::DOWN;
					break;
				case 'f':
				case 'F':
					c.y = ButtonState::DOWN;
					break;
				case 'g':
				case 'G':
					c.x = ButtonState::DOWN;
					break;
				case SDLK_ESCAPE:
					c.back = ButtonState::DOWN;
					break;
				}
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case 'w':
				case 'W':
					ec.leftStickUp = ButtonState::UP;
					break;
				case 'a':
				case 'A':
					ec.leftStickLeft = ButtonState::UP;
					break;
				case 's':
				case 'S':
					ec.leftStickDown = ButtonState::UP;
					break;
				case 'd':
				case 'D':
					ec.leftStickRight = ButtonState::UP;
					break;
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
					ec.shiftPressed = ButtonState::UP;
					break;
				case SDLK_UP:
					ec.rightStickUp = ButtonState::UP;
					break;
				case SDLK_DOWN:
					ec.rightStickDown = ButtonState::UP;
					break;
				case SDLK_LEFT:
					ec.rightStickLeft = ButtonState::UP;
					break;
				case SDLK_RIGHT:
					ec.rightStickRight = ButtonState::UP;
					break;
				case 'q':
				case 'Q':
					c.leftShoulder = ButtonState::UP;
					break;
				case 'e':
				case 'E':
					c.rightShoulder = ButtonState::UP;
					break;
				case 'f':
				case 'F':
					c.y = ButtonState::UP;
					break;
				case 'g':
				case 'G':
					c.x = ButtonState::UP;
					break;
				case SDLK_ESCAPE:
					c.back = ButtonState::UP;
					break;
				}
				break;
			}
		}

		// Set left stick
		vec2 leftStick = vec2(0.0f);
		if (ec.leftStickUp != ButtonState::NOT_PRESSED) leftStick.y = 1.0f;
		else if (ec.leftStickDown != ButtonState::NOT_PRESSED) leftStick.y = -1.0f;
		if (ec.leftStickLeft != ButtonState::NOT_PRESSED) leftStick.x = -1.0f;
		else if (ec.leftStickRight != ButtonState::NOT_PRESSED) leftStick.x = 1.0f;

		leftStick = safeNormalize(leftStick);
		if (ec.shiftPressed != ButtonState::NOT_PRESSED) leftStick *= 0.5f;

		ec.state.leftStick = leftStick;

		// Set right stick
		vec2 rightStick = rawMouse.motion * 200.0f;

		if (ec.rightStickUp != ButtonState::NOT_PRESSED) rightStick.y += 1.0f;
		else if (ec.rightStickDown != ButtonState::NOT_PRESSED) rightStick.y += -1.0f;
		if (ec.rightStickLeft != ButtonState::NOT_PRESSED) rightStick.x += -1.0f;
		else if (ec.rightStickRight != ButtonState::NOT_PRESSED) rightStick.x += 1.0f;

		if (ec.shiftPressed != ButtonState::NOT_PRESSED) rightStick *= 0.5f;
		ec.state.rightStick = rightStick;

		// Set triggers
		if (rawMouse.leftButton == ButtonState::NOT_PRESSED) {
			mEmulatedController.state.rightTrigger = 0.0f;
		} else {
			mEmulatedController.state.rightTrigger = 1.0f;
		}
		if (rawMouse.rightButton == ButtonState::NOT_PRESSED) {
			mEmulatedController.state.leftTrigger = 0.0f;
		} else {
			mEmulatedController.state.leftTrigger = 1.0f;
		}
	}
};

// TestbedLogic creation function
// ------------------------------------------------------------------------------------------------

UniquePtr<GameLogic> createTestbedLogic(Allocator* allocator) noexcept
{
	return sfz::makeUnique<TestbedLogic>(allocator);
}
