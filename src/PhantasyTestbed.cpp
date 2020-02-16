#include <imgui.h>

#include <skipifzero.hpp>
#include <skipifzero_smart_pointers.hpp>

#include <sfz/Context.hpp>
#include <sfz/PhantasyEngineMain.hpp>
#include <sfz/config/GlobalConfig.hpp>
#include <sfz/console/Console.hpp>
#include <sfz/Logging.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/math/Matrix.hpp>
#include <sfz/math/ProjectionMatrices.hpp>
#include <sfz/renderer/Renderer.hpp>
#include <sfz/renderer/BuiltinShaderTypes.hpp>
#include <sfz/renderer/CascadedShadowMaps.hpp>
#include <sfz/rendering/FullscreenTriangle.hpp>
#include <sfz/rendering/ImguiSupport.hpp>
#include <sfz/rendering/SphereLight.hpp>
#include <sfz/state/GameState.hpp>
#include <sfz/state/GameStateEditor.hpp>
#include <sfz/sdl/ButtonState.hpp>
#include <sfz/util/FixedTimeUpdateHelpers.hpp>
#include <sfz/util/GltfLoader.hpp>
#include <sfz/util/GltfWriter.hpp>

#include <ZeroG-cpp.hpp>

#include "Cube.hpp"

#if defined(_WIN32) && defined(NDEBUG)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

using namespace sfz;
using namespace sfz;
using namespace sfz::sdl;

// Helper structs
// ------------------------------------------------------------------------------------------------

struct CameraData {
	sfz::vec3 pos = sfz::vec3(0.0f);
	sfz::vec3 dir = sfz::vec3(0.0f);
	sfz::vec3 up = sfz::vec3(0.0f);
	float near = 0.0f;
	float far = 0.0f;
	float vertFovDeg = 0.0f;
};

struct RenderEntity final {
	sfz::Quaternion rotation = sfz::Quaternion::identity();
	sfz::vec3 scale = sfz::vec3(1.0f);
	sfz::vec3 translation = sfz::vec3(0.0f);
	StringID meshId = StringID::invalid();

	sfz::mat34 transform() const
	{
		// Apply rotation first
		sfz::mat34 tmp = rotation.toMat34();

		// Matrix multiply in scale (order does not matter)
		sfz::vec4 scaleVec = sfz::vec4(scale, 1.0f);
		tmp.row0 *= scaleVec;
		tmp.row1 *= scaleVec;
		tmp.row2 *= scaleVec;

		// Add translation (last)
		tmp.setColumn(3, translation);

		return tmp;
	}
};

struct StaticScene final {
	sfz::Array<RenderEntity> renderEntities;
	sfz::Array<phSphereLight> sphereLights;
};

struct EmulatedGameController {
	sfz::sdl::GameControllerState state;

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

// ECS component types
// ------------------------------------------------------------------------------------------------

constexpr uint32_t RENDER_ENTITY_TYPE = 1u << 0u;
constexpr uint32_t SPHERE_LIGHT_TYPE = 1u << 1u;

// PhantasyTestbedState
// ------------------------------------------------------------------------------------------------

struct PhantasyTestbedState final {
	// Gameloop stuff
	sfz::Console console;
	sfz::FixedTimeStepper fixedTimeStepper;

	CameraData mCam;
	StaticScene mStaticScene;

	EmulatedGameController mEmulatedController;
	sfz::GameControllerState mCtrl;

	Setting* mShowImguiDemo = nullptr;
	sfz::GameStateContainer mGameStateContainer;
	sfz::GameStateEditor mGameStateEditor;
};

// Helper functions
// ------------------------------------------------------------------------------------------------

static void setDir(CameraData& cam, vec3 direction, vec3 up) noexcept
{
	cam.dir = normalize(direction);
	cam.up = normalize(up - dot(up, cam.dir) * cam.dir);
	//sfz_assert_debug(approxEqual(dot(mCam.dir, mCam.up), 0.0f));
}

static void updateEmulatedController(
	PhantasyTestbedState& state,
	const sfz::Array<SDL_Event>& events,
	const Mouse& rawMouse) noexcept
{
	EmulatedGameController& ec = state.mEmulatedController;
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

	leftStick = normalizeSafe(leftStick);
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
		state.mEmulatedController.state.rightTrigger = 0.0f;
	}
	else {
		state.mEmulatedController.state.rightTrigger = 1.0f;
	}
	if (rawMouse.rightButton == ButtonState::NOT_PRESSED) {
		state.mEmulatedController.state.leftTrigger = 0.0f;
	}
	else {
		state.mEmulatedController.state.leftTrigger = 1.0f;
	}
}

// Game loop functions
// ------------------------------------------------------------------------------------------------

static void onInit(
	sfz::Renderer* renderer,
	void* userPtr)
{
	PhantasyTestbedState& state = *static_cast<PhantasyTestbedState*>(userPtr);
	sfz::StringCollection& resStrings = getResourceStrings();

	// Initialize console
	constexpr const char* windows[1] = {
		"Game State Editor"
	};
	state.console.init(getDefaultAllocator(), 1, windows);

	// Load renderer config
	bool rendererLoadConfigSuccess =
		renderer->loadConfiguration("res_ph/shaders/default_renderer_config.json");
	sfz_assert(rendererLoadConfigSuccess);

	// Create fullscreen triangle
	sfz::Mesh fullscreenTriangle = sfz::createFullscreenTriangle(getDefaultAllocator());
	renderer->uploadMeshBlocking(
		resStrings.getStringID("FullscreenTriangle"), fullscreenTriangle);

	// Create game state
	const uint32_t NUM_SINGLETONS = 1;
	const uint32_t SINGLETON_SIZES[NUM_SINGLETONS] = {
		sizeof(RenderEntity)
	};
	const uint32_t MAX_NUM_ENTITIES = 100;
	const uint32_t NUM_COMPONENT_TYPES = 2;
	const uint32_t COMPONENT_SIZES[NUM_COMPONENT_TYPES] = {
		sizeof(RenderEntity),
		sizeof(phSphereLight)
	};
	state.mGameStateContainer = sfz::createGameState(
		NUM_SINGLETONS, SINGLETON_SIZES, MAX_NUM_ENTITIES, NUM_COMPONENT_TYPES, COMPONENT_SIZES);

	// Init ECS viewer
	SingletonInfo singletonInfos[NUM_SINGLETONS];

	singletonInfos[0].singletonIndex = 0;
	singletonInfos[0].singletonName.appendf("phRenderEntity");
	singletonInfos[0].singletonEditor =
		[](uint8_t* userPtr, uint8_t* singletonData, GameStateHeader* state) {

		(void)userPtr;
		(void)state;
		RenderEntity& renderEntity = *reinterpret_cast<RenderEntity*>(singletonData);

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
	componentInfos[0].componentName.appendf("phRenderEntity");
	componentInfos[0].componentEditor =
		[](uint8_t* editorState, uint8_t* componentData, GameStateHeader* state, uint32_t entity) {

		(void)editorState;
		(void)state;
		(void)entity;
		RenderEntity& renderEntity = *reinterpret_cast<RenderEntity*>(componentData);

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
	componentInfos[1].componentName.appendf("phSphereLight");
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

	state.mGameStateEditor.init(
		"Game State Editor", singletonInfos, NUM_SINGLETONS, componentInfos, NUM_COMPONENT_TYPES);

	// Load cube mesh
	StringID cubeMeshId = resStrings.getStringID("virtual/cube");
	sfz::Mesh cubeMesh = createCubeMesh(getDefaultAllocator());
	renderer->uploadMeshBlocking(cubeMeshId, cubeMesh);

	{
		StringID sponzaId = resStrings.getStringID("res/sponza.gltf");

		// Load sponza level
		Mesh mesh;
		sfz::Array<ImageAndPath> textures;
		{
			bool success = loadAssetsFromGltf(
				"res/sponza.gltf",
				mesh,
				textures,
				sfz::getDefaultAllocator(),
				nullptr,
				nullptr);
			if (!success) {
				SFZ_ERROR("PhantasyTesbed", "%s", "Failed to load assets from gltf!");
			}
		}

		// Upload sponza textures to Renderer
		for (const ImageAndPath& item : textures) {
			if (!renderer->textureLoaded(item.globalPathId)) {
				bool success =
					renderer->uploadTextureBlocking(item.globalPathId, item.image, true);
				sfz_assert(success);
			}
		}

		// Upload sponza mesh to Renderer
		bool sponzaUploadSuccess =
			renderer->uploadMeshBlocking(sponzaId, mesh);
		sfz_assert(sponzaUploadSuccess);

		// Create RenderEntity
		StaticScene& staticScene = state.mStaticScene;
		staticScene.renderEntities.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));
		staticScene.sphereLights.init(0, sfz::getDefaultAllocator(), sfz_dbg(""));
		{
			RenderEntity entity;
			entity.meshId = sponzaId;
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
	}

	// Initialize camera
	state.mCam.pos = vec3(3.0f, 3.0f, 3.0f);
	state.mCam.dir = sfz::normalize(vec3(-1.0f, -0.25f, -1.0f));
	state.mCam.up = vec3(0.0f, 1.0f, 0.0f);
	state.mCam.near = 0.05f;
	state.mCam.far = 200.0f;
	state.mCam.vertFovDeg = 60.0f;

	// Common game state stuff
	GameStateHeader* ecs = state.mGameStateContainer.getHeader();

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
		RenderEntity renderEntity;
		renderEntity.meshId = cubeMeshId;
		ecs->addComponent(entity, RENDER_ENTITY_TYPE, renderEntity);
	}

	GlobalConfig& cfg = sfz::getGlobalConfig();
	state.mShowImguiDemo = cfg.sanitizeBool("PhantasyTestbed", "showImguiDemo", true, false);
#if defined(SFZ_IOS)
	cfg.getSetting("Console", "active")->setBool(true);
	cfg.getSetting("Console", "alwaysShowPerformance")->setBool(true);
#endif
}

static sfz::UpdateOp onUpdate(
	sfz::Renderer* renderer,
	float deltaSecs,
	const sfz::UserInput* input,
	void* userPtr)
{
	PhantasyTestbedState& state = *static_cast<PhantasyTestbedState*>(userPtr);

	// Enable/disable console if console key is pressed
	for (const SDL_Event& event : input->events) {
		if (event.type != SDL_KEYUP) continue;
		if (event.key.keysym.sym == '`' ||
			event.key.keysym.sym == '~' ||
			event.key.keysym.sym == SDLK_F1) {

			state.console.toggleActive();
		}
	}

	// Update imgui
	updateImgui(
		renderer->windowResolution(), &input->rawMouse, &input->events, input->controllers.get(0));
	ImGui::NewFrame();

	// Only update stuff if console is not active
	if (!state.console.active()) {

		for (const SDL_Event& event : input->events) {
			if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE) {
				return UpdateOp::QUIT;
			}
		}

		// Update gamecontroller
		updateEmulatedController(state, input->events, input->rawMouse);
		uint32_t controllerIndex = 0;
		const GameController* controller = input->controllers.get(controllerIndex);
		bool emulatedController = controller == nullptr;
		state.mCtrl = (emulatedController) ? state.mEmulatedController.state : controller->state();

		// Run fixed timestep updates
		state.fixedTimeStepper.runTickUpdates(deltaSecs, [&](float tickTimeSecs) {

			float delta = tickTimeSecs;

			float currentSpeed = 10.0f;
			float turningSpeed = 0.8f * PI;

			CameraData& cam = state.mCam;

			// Triggers
			if (state.mCtrl.leftTrigger > state.mCtrl.triggerDeadzone) {
				currentSpeed += (state.mCtrl.leftTrigger * 25.0f);
			}
			if (state.mCtrl.rightTrigger > state.mCtrl.triggerDeadzone) {

			}

			// Analogue Sticks
			if (length(state.mCtrl.rightStick) > state.mCtrl.stickDeadzone) {
				vec3 right = normalize(cross(cam.dir, cam.up));
				mat3 xTurn = mat3::rotation3(vec3(0.0f, -1.0f, 0.0f), state.mCtrl.rightStick[0] * turningSpeed * delta);
				mat3 yTurn = mat3::rotation3(right, state.mCtrl.rightStick[1] * turningSpeed * delta);
				setDir(cam, yTurn * xTurn * cam.dir, yTurn * xTurn * cam.up);
			}
			if (length(state.mCtrl.leftStick) > state.mCtrl.stickDeadzone) {
				vec3 right = normalize(cross(cam.dir, cam.up));
				cam.pos += ((cam.dir * state.mCtrl.leftStick[1] + right * state.mCtrl.leftStick[0]) * currentSpeed * delta);
			}

			// Control Pad
			if (state.mCtrl.padUp == ButtonState::DOWN) {

			}
			else if (state.mCtrl.padDown == ButtonState::DOWN) {

			}
			else if (state.mCtrl.padLeft == ButtonState::DOWN) {

			}
			else if (state.mCtrl.padRight == ButtonState::DOWN) {

			}

			// Shoulder buttons
			if (state.mCtrl.leftShoulder == ButtonState::DOWN || state.mCtrl.leftShoulder == ButtonState::HELD) {
				cam.pos -= vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
			}
			else if (state.mCtrl.rightShoulder == ButtonState::DOWN || state.mCtrl.rightShoulder == ButtonState::HELD) {
				cam.pos += vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
			}

			// Face buttons
			if (state.mCtrl.y == ButtonState::UP) {
			}
			if (state.mCtrl.x == ButtonState::UP) {
			}
			if (state.mCtrl.b == ButtonState::UP) {
			}
			if (state.mCtrl.a == ButtonState::UP) {
			}

			// Face buttons
			if (state.mCtrl.y == ButtonState::DOWN) {
			}
			if (state.mCtrl.x == ButtonState::DOWN) {
			}
			if (state.mCtrl.b == ButtonState::DOWN) {
			}
			if (state.mCtrl.a == ButtonState::DOWN) {
			}

			// Menu buttons
			if (state.mCtrl.back == ButtonState::UP) {
				//return UpdateOp::QUIT();
			}

			setDir(cam, cam.dir, vec3(0.0f, 1.0f, 0.0f));
		});
	}

	// Begin renderer frame
	renderer->frameBegin();

	StringCollection& resStrings = sfz::getResourceStrings();

	// Grab common ECS stuff
	GameStateHeader* gameState = state.mGameStateContainer.getHeader();
	ComponentMask* masks = gameState->componentMasks();

	// Calculate view and projection matrices
	const vec2_i32 windowRes = renderer->windowResolution();
	const float aspect = float(windowRes.x) / float(windowRes.y);

	mat4 viewMatrix;
	zg::createViewMatrix(
		viewMatrix.data(),
		state.mCam.pos.data(),
		state.mCam.dir.data(),
		state.mCam.up.data());

	mat4 projMatrix;
	zg::createPerspectiveProjectionReverseInfinite(
		projMatrix.data(), state.mCam.vertFovDeg, aspect, state.mCam.near);

	const mat4 invProjMatrix = sfz::inverse(projMatrix);

	// Create list of point lights
	sfz::ForwardShaderPointLightsBuffer shaderPointLights;
	for (const phSphereLight& sphereLight : state.mStaticScene.sphereLights) {

		sfz::ShaderPointLight& pointLight =
			shaderPointLights.pointLights[shaderPointLights.numPointLights];
		shaderPointLights.numPointLights += 1;

		pointLight.posVS = transformPoint(viewMatrix, vec3(sphereLight.pos));
		pointLight.range = sphereLight.range;
		pointLight.strength = vec3(sphereLight.color) * (1.0f / 255.0f) * sphereLight.strength;
	}
	phSphereLight* sphereLights = gameState->components<phSphereLight>(SPHERE_LIGHT_TYPE);
	ComponentMask sphereLightyMask =
		ComponentMask::activeMask() | ComponentMask::fromType(SPHERE_LIGHT_TYPE);
	for (uint32_t entity = 0; entity < gameState->maxNumEntities; entity++) {
		if (!masks[entity].fulfills(sphereLightyMask)) continue;

		const phSphereLight& sphereLight = sphereLights[entity];

		sfz::ShaderPointLight& pointLight =
			shaderPointLights.pointLights[shaderPointLights.numPointLights];
		shaderPointLights.numPointLights += 1;

		pointLight.posVS = transformPoint(viewMatrix, vec3(sphereLight.pos));
		pointLight.range = sphereLight.range;
		pointLight.strength = vec3(sphereLight.color) * (1.0f / 255.0f) * sphereLight.strength;
	}

	StringID fullscreenTriangleId = resStrings.getStringID("FullscreenTriangle");

	const sfz::MeshRegisters noRegisters;


	// Lambda for rendering all geometry
	// --------------------------------------------------------------------------------------------

	auto renderGeometry = [&](const sfz::MeshRegisters& registers, mat4 viewMatrix) {
		// Static scene
		for (const RenderEntity& entity : state.mStaticScene.renderEntities) {

			mat4 modelMatrix = mat4(entity.transform());

			// Calculate modelView and normal matrix
			struct {
				mat4 modelViewMatrix;
				mat4 normalMatrix;
			} dynMatrices;

			dynMatrices.modelViewMatrix = viewMatrix * modelMatrix;
			dynMatrices.normalMatrix = sfz::inverse(sfz::transpose(dynMatrices.modelViewMatrix));

			// Render mesh
			renderer->stageSetPushConstant(1, dynMatrices);
			renderer->stageDrawMesh(entity.meshId, registers);
		}

		// Dynamic objects
		RenderEntity* renderEntities = gameState->components<RenderEntity>(RENDER_ENTITY_TYPE);
		ComponentMask renderEntityMask =
			ComponentMask::activeMask() | ComponentMask::fromType(RENDER_ENTITY_TYPE);
		for (uint32_t entityId = 0; entityId < gameState->maxNumEntities; entityId++) {
			if (!masks[entityId].fulfills(renderEntityMask)) continue;
			const RenderEntity& entity = renderEntities[entityId];

			mat4 modelMatrix = mat4(entity.transform());

			// Calculate modelView and normal matrix
			struct {
				mat4 modelViewMatrix;
				mat4 normalMatrix;
			} dynMatrices;

			dynMatrices.modelViewMatrix = viewMatrix * modelMatrix;
			dynMatrices.normalMatrix = sfz::inverse(sfz::transpose(dynMatrices.modelViewMatrix));

			// Render mesh
			renderer->stageSetPushConstant(1, dynMatrices);
			renderer->stageDrawMesh(entity.meshId, registers);
		}
	};


	// GBuffer and directional shadow map pass
	// --------------------------------------------------------------------------------------------

	{
		StringID gbufferStageName = resStrings.getStringID("GBuffer Pass");
		renderer->stageBeginInput(gbufferStageName);

		// Set projection matrix push constant
		renderer->stageSetPushConstant(0, projMatrix);

		sfz::MeshRegisters registers;
		registers.materialIdxPushConstant = 2;
		registers.materialsArray = 3;
		registers.albedo = 0;
		registers.metallicRoughness = 1;
		registers.emissive = 2;

		// Draw geometry
		renderGeometry(registers, viewMatrix);

		renderer->stageEndInput();
	}

	// Calculate cascaded shadow map info
	const vec3 dirLightDirWS = sfz::normalize(vec3(0.0f, -1.0f, 0.1f));
	sfz::CascadedShadowMapInfo cascadedInfo;
	{
		constexpr uint32_t NUM_LEVELS = 3;
		constexpr float LEVEL_DISTS[NUM_LEVELS] = {
			24.0f,
			64.0f,
			128.0f
		};
		cascadedInfo = sfz::calculateCascadedShadowMapInfo(
			state.mCam.pos,
			state.mCam.dir,
			state.mCam.up,
			state.mCam.vertFovDeg,
			aspect,
			state.mCam.near,
			viewMatrix,
			dirLightDirWS,
			80.0f,
			NUM_LEVELS,
			LEVEL_DISTS);
	}

	{
		StringID stageName = resStrings.getStringID("Directional Shadow Map Pass 1");
		renderer->stageBeginInput(stageName);

		// Set push constants
		renderer->stageSetPushConstant(0, cascadedInfo.projMatrices[0]);

		// Draw geometry
		renderGeometry(noRegisters, cascadedInfo.viewMatrices[0]);

		renderer->stageEndInput();
	}

	{
		StringID stageName = resStrings.getStringID("Directional Shadow Map Pass 2");
		renderer->stageBeginInput(stageName);

		// Set push constants
		renderer->stageSetPushConstant(0, cascadedInfo.projMatrices[1]);

		// Draw geometry
		renderGeometry(noRegisters, cascadedInfo.viewMatrices[1]);

		renderer->stageEndInput();
	}

	{
		StringID stageName = resStrings.getStringID("Directional Shadow Map Pass 3");
		renderer->stageBeginInput(stageName);

		// Set push constants
		renderer->stageSetPushConstant(0, cascadedInfo.projMatrices[2]);

		// Draw geometry
		renderGeometry(noRegisters, cascadedInfo.viewMatrices[2]);

		renderer->stageEndInput();
	}


	// Directional and Point Light Shading
	// --------------------------------------------------------------------------------------------

	{
		bool success = renderer->stageBarrierProgressNext();
		sfz_assert(success);
	}

	// Directional shading
	{

		// Begin input
		StringID stageName = resStrings.getStringID("Directional Shading Pass");
		renderer->stageBeginInput(stageName);

		// Set constant buffers
		renderer->stageSetPushConstant(0, invProjMatrix);

		struct {
			sfz::DirectionalLight dirLight;
			mat4 lightMatrix1;
			mat4 lightMatrix2;
			mat4 lightMatrix3;
			float levelDist1;
			float levelDist2;
			float levelDist3;
			float ___PADDING___;
		} lightInfo;
		lightInfo.dirLight.lightDirVS = sfz::transformDir(viewMatrix, dirLightDirWS);
		lightInfo.dirLight.strength = vec3(10.0f);
		lightInfo.lightMatrix1 = cascadedInfo.lightMatrices[0];
		lightInfo.lightMatrix2 = cascadedInfo.lightMatrices[1];
		lightInfo.lightMatrix3 = cascadedInfo.lightMatrices[2];
		lightInfo.levelDist1 = cascadedInfo.levelDists[0];
		lightInfo.levelDist2 = cascadedInfo.levelDists[1];
		lightInfo.levelDist3 = cascadedInfo.levelDists[2];
		renderer->stageSetConstantBuffer(1, lightInfo);

		// Fullscreen pass
		renderer->stageDrawMesh(fullscreenTriangleId, noRegisters);

		renderer->stageEndInput();
	}

	// Point lights
	{

		// Begin input
		StringID stageName = resStrings.getStringID("Point Light Shading Pass");
		renderer->stageBeginInput(stageName);

		// Set push constants
		renderer->stageSetPushConstant(0, invProjMatrix);

		// Set constant buffers
		renderer->stageSetConstantBuffer(1, shaderPointLights);

		// Fullscreen pass
		renderer->stageDrawMesh(fullscreenTriangleId, noRegisters);

		renderer->stageEndInput();
	}


	// Copy Out Pass
	// --------------------------------------------------------------------------------------------

	{
		bool success = renderer->stageBarrierProgressNext();
		sfz_assert(success);

		// Begin input
		StringID copyOutPassName = resStrings.getStringID("Copy Out Pass");
		renderer->stageBeginInput(copyOutPassName);

		// Set window resolution push constant
		vec4_u32 pushConstantRes = vec4_u32(uint32_t(windowRes.x), uint32_t(windowRes.y), 0u, 0u);
		renderer->stageSetPushConstant(0, pushConstantRes);

		// Fullscreen pass
		renderer->stageDrawMesh(fullscreenTriangleId, noRegisters);

		renderer->stageEndInput();
	}

	// Update console and inject testbed specific windows
	state.console.render(deltaSecs * 1000.0f);
	if (state.console.active()) {

		// View of ECS system
		sfz::GameStateHeader* gameState = state.mGameStateContainer.getHeader();
		ImGui::SetNextWindowPos(vec2(700.0f, 00.0f), ImGuiCond_FirstUseEver);
		state.mGameStateEditor.render(gameState);

		// Render Renderer's UI
		renderer->renderImguiUI();
	}
	else {
		if (state.mShowImguiDemo->boolValue()) ImGui::ShowDemoWindow();
	}

	// Finish rendering frame
	renderer->frameFinish();

	return sfz::UpdateOp::NO_OP;
}

static void onQuit(void* userPtr)
{
	PhantasyTestbedState* state = static_cast<PhantasyTestbedState*>(userPtr);
	sfz::getDefaultAllocator()->deleteObject(state);
}


// Phantasy Testbed's main function
// ------------------------------------------------------------------------------------------------

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
	options.userPtr = sfz::getDefaultAllocator()->
		newObject<PhantasyTestbedState>(sfz_dbg("PhantasyTestbedState"));
	options.initFunc = onInit;
	options.updateFunc = onUpdate;
	options.quitFunc = onQuit;
	return options;
}
