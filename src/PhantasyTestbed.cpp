#include <imgui.h>

#include <skipifzero.hpp>
#include <skipifzero_math.hpp>
#include <skipifzero_smart_pointers.hpp>
#include <skipifzero_strings.hpp>

#include <sfz/Context.hpp>
#include <sfz/PhantasyEngineMain.hpp>
#include <sfz/config/GlobalConfig.hpp>
#include <sfz/debug/Console.hpp>
#include <sfz/Logging.hpp>
#include <sfz/renderer/Renderer.hpp>
#include <sfz/renderer/BuiltinShaderTypes.hpp>
#include <sfz/renderer/CascadedShadowMaps.hpp>
#include <sfz/rendering/FullscreenTriangle.hpp>
#include <sfz/rendering/ImguiSupport.hpp>
#include <sfz/rendering/SphereLight.hpp>
#include <sfz/state/GameState.hpp>
#include <sfz/state/GameStateContainer.hpp>
#include <sfz/state/GameStateEditor.hpp>
#include <sfz/util/FixedTimeUpdateHelpers.hpp>
#include <sfz/util/GltfLoader.hpp>
#include <sfz/util/GltfWriter.hpp>

#include <ZeroG.h>

#include "Cube.hpp"

#if defined(_WIN32) && defined(NDEBUG)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

using namespace sfz;

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
	sfz::quat rotation = sfz::quat::identity();
	sfz::vec3 scale = sfz::vec3(1.0f);
	sfz::vec3 translation = sfz::vec3(0.0f);
	strID meshId;

	sfz::mat34 transform() const
	{
		// Apply rotation first
		sfz::mat34 tmp = rotation.toMat34();

		// Matrix multiply in scale (order does not matter)
		sfz::vec4 scaleVec = sfz::vec4(scale, 1.0f);
		tmp.row(0) *= scaleVec;
		tmp.row(1) *= scaleVec;
		tmp.row(2) *= scaleVec;

		// Add translation (last)
		tmp.setColumn(3, translation);

		return tmp;
	}
};

struct StaticScene final {
	sfz::Array<RenderEntity> renderEntities;
	sfz::Array<phSphereLight> sphereLights;
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

	sfz::RawInputState prevInput = {};

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

// Game loop functions
// ------------------------------------------------------------------------------------------------

static void onInit(void* userPtr)
{
	PhantasyTestbedState& state = *static_cast<PhantasyTestbedState*>(userPtr);
	sfz::Renderer& renderer = sfz::getRenderer();

	// Initialize console
	constexpr const char* windows[1] = {
		"Game State Editor"
	};
	state.console.init(getDefaultAllocator(), 1, windows);

	// Load renderer config
	bool rendererLoadConfigSuccess =
		renderer.loadConfiguration("res_ph/shaders/default_renderer_config.json");
	sfz_assert(rendererLoadConfigSuccess);

	// Create fullscreen triangle
	sfz::Mesh fullscreenTriangle = sfz::createFullscreenTriangle(getDefaultAllocator());
	renderer.uploadMeshBlocking(strID("FullscreenTriangle"), fullscreenTriangle);

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
	state.mGameStateContainer = sfz::GameStateContainer::create(
		NUM_SINGLETONS, SINGLETON_SIZES, MAX_NUM_ENTITIES, NUM_COMPONENT_TYPES, COMPONENT_SIZES, sfz::getDefaultAllocator());

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
			renderEntity.rotation = quat::fromEuler(eulerRot);
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
			renderEntity.rotation = quat::fromEuler(eulerRot);
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
		"Game State Editor", singletonInfos, NUM_SINGLETONS, componentInfos, NUM_COMPONENT_TYPES, sfz::getDefaultAllocator());

	// Load cube mesh
	strID cubeMeshId = strID("virtual/cube");
	sfz::Mesh cubeMesh = createCubeMesh(getDefaultAllocator());
	renderer.uploadMeshBlocking(cubeMeshId, cubeMesh);

	{
		strID sponzaId = strID("res/sponza.gltf");

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
			if (!renderer.textureLoaded(item.globalPathId)) {
				bool success =
					renderer.uploadTextureBlocking(item.globalPathId, item.image, true);
				sfz_assert(success);
			}
		}

		// Upload sponza mesh to Renderer
		bool sponzaUploadSuccess =
			renderer.uploadMeshBlocking(sponzaId, mesh);
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
	float deltaSecs,
	const SDL_Event* events,
	uint32_t numEvents,
	const sfz::RawInputState* rawFrameInput,
	void* userPtr)
{
	PhantasyTestbedState& state = *static_cast<PhantasyTestbedState*>(userPtr);
	sfz::Renderer& renderer = sfz::getRenderer();

	// Enable/disable console if console key is pressed
	for (uint32_t i = 0; i < numEvents; i++) {
		const SDL_Event& event = events[i];
		if (event.type != SDL_KEYUP) continue;
		if (event.key.keysym.sym == '`' ||
			event.key.keysym.sym == '~' ||
			event.key.keysym.sym == SDLK_F1) {

			state.console.toggleActive();
		}
	}

	// Update imgui
	updateImgui(vec2_i32(rawFrameInput->windowDims), *rawFrameInput, events, numEvents);
	ImGui::NewFrame();

	// Only update stuff if console is not active
	if (!state.console.active()) {

		for (uint32_t i = 0; i < numEvents; i++) {
			const SDL_Event& event = events[i];
			if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE) {
				return UpdateOp::QUIT;
			}
		}

		// Run fixed timestep updates
		state.fixedTimeStepper.runTickUpdates(deltaSecs, [&](float tickTimeSecs) {

			float delta = tickTimeSecs;

			float currentSpeed = 10.0f;
			float turningSpeed = 0.8f * PI;

			CameraData& cam = state.mCam;

			const sfz::KeyboardState& kb = rawFrameInput->kb;
			const sfz::MouseState& mouse = rawFrameInput->mouse;

			if (kb.scancodes[SDL_SCANCODE_LSHIFT]) currentSpeed = 25.0f;

			if (mouse.delta != vec2_i32(0)) {
				vec2 mouseDelta = vec2(mouse.delta) * 0.1f;
				vec3 right = normalize(cross(cam.dir, cam.up));
				mat3 xTurn = mat3::rotation3(vec3(0.0f, -1.0f, 0.0f), mouseDelta[0] * turningSpeed * delta);
				mat3 yTurn = mat3::rotation3(right, mouseDelta[1] * turningSpeed * delta);
				setDir(cam, yTurn * xTurn * cam.dir, yTurn * xTurn * cam.up);
			}

			// x and y-axis in range [-1, 1]
			vec2 movement = vec2(0.0f);
			movement.x = float(kb.scancodes[SDL_SCANCODE_D]) - float(kb.scancodes[SDL_SCANCODE_A]);
			movement.y = float(kb.scancodes[SDL_SCANCODE_W]) - float(kb.scancodes[SDL_SCANCODE_S]);

			// Normalize movement (i.e. length(movement) <= 1)
			movement = sfz::normalizeSafe(movement);
			
			if (length(movement) > 0.1f) {
				vec3 right = normalize(cross(cam.dir, cam.up));
				cam.pos += ((cam.dir * movement[1] + right * movement[0]) * currentSpeed * delta);
			}

			if (kb.scancodes[SDL_SCANCODE_Q]) {
				cam.pos -= vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
			}
			if (kb.scancodes[SDL_SCANCODE_E]) {
				cam.pos += vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
			}

			setDir(cam, cam.dir, vec3(0.0f, 1.0f, 0.0f));
		});
	}

	// Begin renderer frame
	renderer.frameBegin();

	// Grab common ECS stuff
	GameStateHeader* gameState = state.mGameStateContainer.getHeader();
	ComponentMask* masks = gameState->componentMasks();

	// Calculate view and projection matrices
	const vec2_i32 windowRes = renderer.windowResolution();
	const float aspect = float(windowRes.x) / float(windowRes.y);

	// Calculate internal resolution
	GlobalConfig& cfg = getGlobalConfig();
	const float internalResScale =
		cfg.getSetting("Renderer", "internalResolutionScale")->floatValue();
	const vec2_i32 internalRes = vec2_i32(
		std::round(windowRes.x * internalResScale), std::round(windowRes.y * internalResScale));

	mat4 viewMatrix;
	zgUtilCreateViewMatrix(
		viewMatrix.data(),
		state.mCam.pos.data(),
		state.mCam.dir.data(),
		state.mCam.up.data());

	mat4 projMatrix;
	zgUtilCreatePerspectiveProjectionReverseInfinite(
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

	strID fullscreenTriangleId = strID("FullscreenTriangle");

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
			renderer.stageSetPushConstant(1, dynMatrices);
			renderer.stageDrawMesh(entity.meshId, registers);
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
			renderer.stageSetPushConstant(1, dynMatrices);
			renderer.stageDrawMesh(entity.meshId, registers);
		}
	};


	// GBuffer and directional shadow map pass
	// --------------------------------------------------------------------------------------------

	{
		renderer.stageBeginInput("GBuffer Pass");

		renderer.stageClearDepthBufferOptimal();
		renderer.stageClearRenderTargetsOptimal();

		// Set projection matrix push constant
		renderer.stageSetPushConstant(0, projMatrix);

		sfz::MeshRegisters registers;
		registers.materialIdxPushConstant = 2;
		registers.materialsArray = 3;
		registers.albedo = 0;
		registers.metallicRoughness = 1;
		registers.emissive = 2;

		// Draw geometry
		renderGeometry(registers, viewMatrix);

		renderer.stageEndInput();
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
		renderer.stageBeginInput("Directional Shadow Map Pass 1");

		renderer.stageClearDepthBufferOptimal();

		// Set push constants
		renderer.stageSetPushConstant(0, cascadedInfo.projMatrices[0]);

		// Draw geometry
		renderGeometry(noRegisters, cascadedInfo.viewMatrices[0]);

		renderer.stageEndInput();
	}

	{
		renderer.stageBeginInput("Directional Shadow Map Pass 2");

		renderer.stageClearDepthBufferOptimal();

		// Set push constants
		renderer.stageSetPushConstant(0, cascadedInfo.projMatrices[1]);

		// Draw geometry
		renderGeometry(noRegisters, cascadedInfo.viewMatrices[1]);

		renderer.stageEndInput();
	}

	{
		renderer.stageBeginInput("Directional Shadow Map Pass 3");

		renderer.stageClearDepthBufferOptimal();

		// Set push constants
		renderer.stageSetPushConstant(0, cascadedInfo.projMatrices[2]);

		// Draw geometry
		renderGeometry(noRegisters, cascadedInfo.viewMatrices[2]);

		renderer.stageEndInput();
	}


	// Directional and Point Light Shading
	// --------------------------------------------------------------------------------------------

	{
		bool success = renderer.frameProgressNextStageGroup();
		sfz_assert(success);
	}

	// Directional shading
	{
		renderer.stageBeginInput("Directional Shading Pass");

		// Set constant buffers
		renderer.stageSetPushConstant(0, invProjMatrix);

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
		renderer.stageUploadToStreamingBuffer(
			"Directional Light Const Buffer", (const uint8_t*)&lightInfo, sizeof(lightInfo));

		sfz::PipelineBindings bindings;
		bindings.addConstBuffer("Directional Light Const Buffer", 1);
		bindings.addTexture("GBuffer_albedo", 0);
		bindings.addTexture("GBuffer_metallic_roughness", 1);
		bindings.addTexture("GBuffer_emissive", 2);
		bindings.addTexture("GBuffer_normal", 3);
		bindings.addTexture("GBuffer_depthbuffer", 4);
		bindings.addTexture("ShadowMapCascaded1", 5);
		bindings.addTexture("ShadowMapCascaded2", 6);
		bindings.addTexture("ShadowMapCascaded3", 7);
		bindings.addUnorderedTexture("LightAccumulation1", 0, 0);
		renderer.stageSetBindings(bindings);

		// Fullscreen pass
		// Run one thread per pixel
		const vec2_i32 groupDim = renderer.stageGetComputeGroupDims().xy;
		const vec2_i32 numGroups = (internalRes + groupDim - vec2_i32(1)) / groupDim;
		renderer.stageDispatchComputeNoAutoBindings(numGroups.x, numGroups.y);

		renderer.stageEndInput();
	}

	// Point lights
	{
		renderer.stageBeginInput("Point Light Shading Pass");

		// Set push constants
		renderer.stageSetPushConstant(0, invProjMatrix);

		renderer.stageUploadToStreamingBufferUntyped(
			"Point Lights Buffer", &shaderPointLights, sizeof(uint8_t), sizeof(shaderPointLights));

		sfz::PipelineBindings bindings;
		bindings.addConstBuffer("Point Lights Buffer", 1);
		bindings.addTexture("GBuffer_albedo", 0);
		bindings.addTexture("GBuffer_metallic_roughness", 1);
		bindings.addTexture("GBuffer_normal", 2);
		bindings.addTexture("GBuffer_depthbuffer", 3);
		bindings.addUnorderedTexture("LightAccumulation1", 0, 0);
		renderer.stageSetBindings(bindings);

		// Fullscreen pass
		// Run one thread per pixel
		const vec2_i32 groupDim = renderer.stageGetComputeGroupDims().xy;
		const vec2_i32 numGroups = (internalRes + groupDim - vec2_i32(1)) / groupDim;
		renderer.stageDispatchComputeNoAutoBindings(numGroups.x, numGroups.y);

		renderer.stageEndInput();
	}


	// Copy Out Pass
	// --------------------------------------------------------------------------------------------

	{
		bool success = renderer.frameProgressNextStageGroup();
		sfz_assert(success);
	}

	{
		renderer.stageBeginInput("Copy Out Pass");

		// Set window resolution push constant
		vec4_u32 pushConstantRes = vec4_u32(uint32_t(windowRes.x), uint32_t(windowRes.y), 0u, 0u);
		renderer.stageSetPushConstant(0, pushConstantRes);

		// Fullscreen pass
		renderer.stageDrawMesh(fullscreenTriangleId, noRegisters);

		renderer.stageEndInput();
	}

	// Update console and inject testbed specific windows
	state.console.render();
	if (state.console.active()) {

		// View of ECS system
		sfz::GameStateHeader* gameStateTmp = state.mGameStateContainer.getHeader();
		ImGui::SetNextWindowPos(vec2(700.0f, 00.0f), ImGuiCond_FirstUseEver);
		state.mGameStateEditor.render(gameStateTmp);
	}
	else {
		if (state.mShowImguiDemo->boolValue()) ImGui::ShowDemoWindow();
	}

	// Finish rendering frame
	renderer.frameFinish();

	// Store input as previous input
	state.prevInput = *rawFrameInput;

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
