#include "TestbedLogic.hpp"

#include <imgui.h>

#include <sfz/Logging.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/math/Matrix.hpp>

#include <ph/Context.hpp>
#include <ph/config/GlobalConfig.hpp>
#include <ph/state/GameState.hpp>
#include <ph/state/GameStateEditor.hpp>
#include <ph/sdl/ButtonState.hpp>
#include <ph/util/GltfLoader.hpp>
#include <ph/util/GltfWriter.hpp>

#include "Cube.hpp"

using namespace ph;
using namespace sfz;
using namespace ph::sdl;

// ECS component types
// ------------------------------------------------------------------------------------------------

constexpr uint32_t RENDER_ENTITY_TYPE = 1u << 0u;
constexpr uint32_t SPHERE_LIGHT_TYPE = 1u << 1u;

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
	ph::GameStateContainer mGameStateContainer;
	ph::GameStateEditor mGameStateEditor;

	// Overloaded methods from GameLogic
	// --------------------------------------------------------------------------------------------

	void initialize(UpdateableState& state, Renderer& renderer) override final
	{
		// Create game state
		const uint32_t NUM_SINGLETONS = 1;
		const uint32_t SINGLETON_SIZES[NUM_SINGLETONS] = {
			sizeof(phRenderEntity)
		};
		const uint32_t MAX_NUM_ENTITIES = 100;
		const uint32_t NUM_COMPONENT_TYPES = 2;
		const uint32_t COMPONENT_SIZES[NUM_COMPONENT_TYPES] = {
			sizeof(phRenderEntity),
			sizeof(phSphereLight)
		};
		mGameStateContainer = ph::createGameState(
			NUM_SINGLETONS, SINGLETON_SIZES, MAX_NUM_ENTITIES, NUM_COMPONENT_TYPES, COMPONENT_SIZES);

		// Init ECS viewer
		SingletonInfo singletonInfos[NUM_SINGLETONS];

		singletonInfos[0].singletonIndex = 0;
		singletonInfos[0].singletonName.printf("phRenderEntity");
		singletonInfos[0].singletonEditor =
			[](uint8_t * userPtr, uint8_t * singletonData, GameStateHeader * state) {

			(void)userPtr;
			(void)state;
			phRenderEntity& renderEntity = *reinterpret_cast<phRenderEntity*>(singletonData);

			ImGui::InputFloat3("Scale", renderEntity.scale.data());
			ImGui::InputFloat3("Translation", renderEntity.translation.data());
			if (ImGui::InputFloat4("Rotation quaternion", renderEntity.rotation.vector.data())) {
				renderEntity.rotation = normalize(renderEntity.rotation);
			}
			vec3 eulerRot = renderEntity.rotation.toEuler();
			if (ImGui::InputFloat3("Rotation euler", eulerRot.data())) {
				renderEntity.rotation = Quaternion::fromEuler(eulerRot);
				renderEntity.rotation = normalize(renderEntity.rotation);
			}
		};

		ComponentInfo componentInfos[NUM_COMPONENT_TYPES];

		componentInfos[0].componentType = RENDER_ENTITY_TYPE;
		componentInfos[0].componentName.printf("phRenderEntity");
		componentInfos[0].componentEditor =
			[](uint8_t* editorState, uint8_t* componentData, GameStateHeader* state, uint32_t entity) {

			(void)editorState;
			(void)state;
			(void)entity;
			phRenderEntity& renderEntity = *reinterpret_cast<phRenderEntity*>(componentData);

			ImGui::InputFloat3("Scale", renderEntity.scale.data());
			ImGui::InputFloat3("Translation", renderEntity.translation.data());
			if (ImGui::InputFloat4("Rotation quaternion", renderEntity.rotation.vector.data())) {
				renderEntity.rotation = normalize(renderEntity.rotation);
			}
			vec3 eulerRot = renderEntity.rotation.toEuler();
			if (ImGui::InputFloat3("Rotation euler", eulerRot.data())) {
				renderEntity.rotation = Quaternion::fromEuler(eulerRot);
				renderEntity.rotation = normalize(renderEntity.rotation);
			}
		};

		componentInfos[1].componentType = SPHERE_LIGHT_TYPE;
		componentInfos[1].componentName.printf("phSphereLight");
		componentInfos[1].componentEditor =
			[](uint8_t* editorState, uint8_t* componentData, GameStateHeader* state, uint32_t entity) {

			(void)editorState;
			(void)state;
			(void)entity;
			phSphereLight& sphereLight = *reinterpret_cast<phSphereLight*>(componentData);

			ImGui::InputFloat3("Position", sphereLight.pos.data());
			ImGui::InputFloat("Radius", &sphereLight.radius);
			ImGui::InputFloat("Range", &sphereLight.range);
			ImGui::InputFloat("Strength", &sphereLight.strength);
			vec3 color = vec3(sphereLight.color) * (1.0f / 255.0f);
			if (ImGui::ColorEdit3("Color", color.data())) {
				color *= 255.0f;
				color += vec3(0.5f); // To round properly
				sphereLight.color = vec3_u8(uint8_t(color.x), uint8_t(color.y), uint8_t(color.z));
			}
		};

		mGameStateEditor.init(
			"Game State Editor", singletonInfos, NUM_SINGLETONS, componentInfos, NUM_COMPONENT_TYPES);

		{
			// Load sponza level
			Mesh mesh;
			DynArray<ImageAndPath> textures;
			bool success = loadAssetsFromGltf(
				"res/sponza.gltf",
				mesh,
				textures,
				sfz::getDefaultAllocator(),
				&state.resourceManager);
			if (!success) {
				SFZ_ERROR("PhantasyTesbed", "%s", "Failed to load assets from gltf!");
			}

			// Upload sponza level to Renderer via resource manager
			state.resourceManager.registerMesh("res/sponza.gltf", mesh, textures);

			// Create RenderEntity
			StaticScene staticScene;
			staticScene.renderEntities.create(1);
			{
				phRenderEntity entity;
				entity.meshIndex = 0;
				staticScene.renderEntities.add(entity);
			}

			// Add a static light
			phSphereLight tmpLight;
			tmpLight.pos = vec3(0.0f, 3.0f, 0.0f);
			tmpLight.range = 70.0f;
			tmpLight.radius = 0.5f;
			tmpLight.color = vec3_u8(255);
			tmpLight.strength = 150.0f;
			tmpLight.bitmaskFlags = SPHERE_LIGHT_STATIC_SHADOWS_BIT | SPHERE_LIGHT_DYNAMIC_SHADOWS_BIT;
			staticScene.sphereLights.add(tmpLight);

			// Upload static scene to renderer
			renderer.setStaticScene(staticScene);
		}

		// Initialize camera
		state.cam.pos = vec3(3.0f, 3.0f, 3.0f);
		state.cam.dir = sfz::normalize(vec3(-1.0f, -0.25f, -1.0f));
		state.cam.up =  vec3(0.0f, 1.0f, 0.0f);
		state.cam.near = 0.05f;
		state.cam.far = 200.0f;
		state.cam.vertFovDeg = 60.0f;

		// Allocate memory for render entities
		state.renderEntities.create(MAX_NUM_ENTITIES, getDefaultAllocator());

		// Common game state stuff
		GameStateHeader* ecs = mGameStateContainer.getHeader();

		// Add dynamic light entities
		vec3_u8 lightColors[] = {
			vec3_u8(255, 0, 255),
			vec3_u8(255, 255, 255)
		};
		uint32_t numLights = sizeof(lightColors) / sizeof(vec3_u8);
		for (uint32_t i = 0; i < numLights; i++) {
			phSphereLight light;

			light.pos = vec3(-50.0f + 100.0f * i / (numLights - 1), 5.0f, 0.0f);
			light.range = 70.0f;
			light.color = lightColors[i];
			light.strength = 300.0f;
			light.radius = 0.5f;
			light.bitmaskFlags = SPHERE_LIGHT_STATIC_SHADOWS_BIT | SPHERE_LIGHT_DYNAMIC_SHADOWS_BIT;

			Entity lightEntity = ecs->createEntity();
			ecs->addComponent(lightEntity, SPHERE_LIGHT_TYPE, light);
		}

		// Add a box entity
		{
			Entity entity = ecs->createEntity();
			phRenderEntity renderEntity;
			renderEntity.meshIndex = 0;
			ecs->addComponent(entity, RENDER_ENTITY_TYPE, renderEntity);
		}

		GlobalConfig& cfg = ph::getGlobalConfig();
		mShowImguiDemo = cfg.sanitizeBool("PhantasyTestbed", "showImguiDemo", true, false);
#if defined(SFZ_IOS)
		cfg.getSetting("Console", "active")->setBool(true);
		cfg.getSetting("Console", "alwaysShowPerformance")->setBool(true);
#endif
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

	RenderSettings preRenderHook(
		UpdateableState& state, const UpdateInfo& updateInfo, Renderer& renderer) override final
	{
		(void)updateInfo;
		(void)renderer;

		// Grab common ECS stuff
		GameStateHeader* gameState = mGameStateContainer.getHeader();
		ComponentMask* masks = gameState->componentMasks();

		// Copy render entities from ECS to list to draw
		state.renderEntities.clear();
		phRenderEntity* renderEntities = gameState->components<phRenderEntity>(RENDER_ENTITY_TYPE);
		ComponentMask renderEntityMask =
			ComponentMask::activeMask() | ComponentMask::fromType(RENDER_ENTITY_TYPE);
		for (uint32_t entity = 0; entity < gameState->maxNumEntities; entity++) {
			if (!masks[entity].fulfills(renderEntityMask)) continue;
			state.renderEntities.add(renderEntities[entity]);
		}

		// Copy sphere lights from ECS to list to draw
		state.dynamicSphereLights.clear();
		phSphereLight* sphereLights = gameState->components<phSphereLight>(SPHERE_LIGHT_TYPE);
		ComponentMask sphereLightyMask =
			ComponentMask::activeMask() | ComponentMask::fromType(SPHERE_LIGHT_TYPE);
		for (uint32_t entity = 0; entity < gameState->maxNumEntities; entity++) {
			if (!masks[entity].fulfills(sphereLightyMask)) continue;
			state.dynamicSphereLights.add(sphereLights[entity]);
		}

		RenderSettings settings;
		settings.clearColor = vec4(0.0f);
		return settings;
	}

	void renderCustomImgui() override final
	{
		if (mShowImguiDemo->boolValue()) ImGui::ShowDemoWindow();
	}

	void injectConsoleMenu() override final
	{
		// View of ECS system
		ph::GameStateHeader* gameState = mGameStateContainer.getHeader();
		ImGui::SetNextWindowPos(vec2(700.0f, 00.0f), ImGuiCond_FirstUseEver);
		mGameStateEditor.render(gameState);
	}

	uint32_t injectConsoleMenuNumWindowsToDockInitially() override final
	{
		return 1;
	}

	const char* injectConsoleMenuNameOfWindowToDockInitially(uint32_t idx) override final
	{
		(void)idx;
		return "Game State Editor";
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
