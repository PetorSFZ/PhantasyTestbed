#include "TestbedUpdateable.hpp"

#include <imgui.h>

#include <sfz/math/MathSupport.hpp>
#include <sfz/math/Matrix.hpp>
#include <sfz/util/IO.hpp>

#include <ph/config/GlobalConfig.hpp>
#include <ph/utils/Logging.hpp>
#include <ph/rendering/Image.hpp>
#include <ph/rendering/ImguiSupport.hpp>
#include <ph/rendering/Renderer.hpp>

using namespace sfz;
using namespace ph;

// Cube model
// ------------------------------------------------------------------------------------------------

static const vec3 CUBE_POSITIONS[] = {
	// x, y, z
	// Left
	vec3(0.0f, 0.0f, 0.0f), // 0, left-bottom-back
	vec3(0.0f, 0.0f, 1.0f), // 1, left-bottom-front
	vec3(0.0f, 1.0f, 0.0f), // 2, left-top-back
	vec3(0.0f, 1.0f, 1.0f), // 3, left-top-front

	// Right
	vec3(1.0f, 0.0f, 0.0f), // 4, right-bottom-back
	vec3(1.0f, 0.0f, 1.0f), // 5, right-bottom-front
	vec3(1.0f, 1.0f, 0.0f), // 6, right-top-back
	vec3(1.0f, 1.0f, 1.0f), // 7, right-top-front

	// Bottom
	vec3(0.0f, 0.0f, 0.0f), // 8, left-bottom-back
	vec3(0.0f, 0.0f, 1.0f), // 9, left-bottom-front
	vec3(1.0f, 0.0f, 0.0f), // 10, right-bottom-back
	vec3(1.0f, 0.0f, 1.0f), // 11, right-bottom-front

	// Top
	vec3(0.0f, 1.0f, 0.0f), // 12, left-top-back
	vec3(0.0f, 1.0f, 1.0f), // 13, left-top-front
	vec3(1.0f, 1.0f, 0.0f), // 14, right-top-back
	vec3(1.0f, 1.0f, 1.0f), // 15, right-top-front

	// Back
	vec3(0.0f, 0.0f, 0.0f), // 16, left-bottom-back
	vec3(0.0f, 1.0f, 0.0f), // 17, left-top-back
	vec3(1.0f, 0.0f, 0.0f), // 18, right-bottom-back
	vec3(1.0f, 1.0f, 0.0f), // 19, right-top-back

	// Front
	vec3(0.0f, 0.0f, 1.0f), // 20, left-bottom-front
	vec3(0.0f, 1.0f, 1.0f), // 21, left-top-front
	vec3(1.0f, 0.0f, 1.0f), // 22, right-bottom-front
	vec3(1.0f, 1.0f, 1.0f)  // 23, right-top-front
};

static const vec3 CUBE_NORMALS[] = {
	// x, y, z
	// Left
	vec3(-1.0f, 0.0f, 0.0f), // 0, left-bottom-back
	vec3(-1.0f, 0.0f, 0.0f), // 1, left-bottom-front
	vec3(-1.0f, 0.0f, 0.0f), // 2, left-top-back
	vec3(-1.0f, 0.0f, 0.0f), // 3, left-top-front

	// Right
	vec3(1.0f, 0.0f, 0.0f), // 4, right-bottom-back
	vec3(1.0f, 0.0f, 0.0f), // 5, right-bottom-front
	vec3(1.0f, 0.0f, 0.0f), // 6, right-top-back
	vec3(1.0f, 0.0f, 0.0f), // 7, right-top-front

	// Bottom
	vec3(0.0f, -1.0f, 0.0f), // 8, left-bottom-back
	vec3(0.0f, -1.0f, 0.0f), // 9, left-bottom-front
	vec3(0.0f, -1.0f, 0.0f), // 10, right-bottom-back
	vec3(0.0f, -1.0f, 0.0f), // 11, right-bottom-front

	// Top
	vec3(0.0f, 1.0f, 0.0f), // 12, left-top-back
	vec3(0.0f, 1.0f, 0.0f), // 13, left-top-front
	vec3(0.0f, 1.0f, 0.0f), // 14, right-top-back
	vec3(0.0f, 1.0f, 0.0f), // 15, right-top-front

	// Back
	vec3(0.0f, 0.0f, -1.0f), // 16, left-bottom-back
	vec3(0.0f, 0.0f, -1.0f), // 17, left-top-back
	vec3(0.0f, 0.0f, -1.0f), // 18, right-bottom-back
	vec3(0.0f, 0.0f, -1.0f), // 19, right-top-back

	// Front
	vec3(0.0f, 0.0f, 1.0f), // 20, left-bottom-front
	vec3(0.0f, 0.0f, 1.0f), // 21, left-top-front
	vec3(0.0f, 0.0f, 1.0f), // 22, right-bottom-front
	vec3(0.0f, 0.0f, 1.0f)  // 23, right-top-front
};

static const vec2 CUBE_TEXCOORDS[] = {
	// u, v
	// Left
	vec2(0.0f, 0.0f), // 0, left-bottom-back
	vec2(1.0f, 0.0f), // 1, left-bottom-front
	vec2(0.0f, 1.0f), // 2, left-top-back
	vec2(1.0f, 1.0f), // 3, left-top-front

	// Right
	vec2(1.0f, 0.0f), // 4, right-bottom-back
	vec2(0.0f, 0.0f), // 5, right-bottom-front
	vec2(1.0f, 1.0f), // 6, right-top-back
	vec2(0.0f, 1.0f), // 7, right-top-front

	// Bottom
	vec2(0.0f, 0.0f), // 8, left-bottom-back
	vec2(0.0f, 1.0f), // 9, left-bottom-front
	vec2(1.0f, 0.0f), // 10, right-bottom-back
	vec2(1.0f, 1.0f), // 11, right-bottom-front

	// Top
	vec2(0.0f, 1.0f), // 12, left-top-back
	vec2(0.0f, 0.0f), // 13, left-top-front
	vec2(1.0f, 1.0f), // 14, right-top-back
	vec2(1.0f, 0.0f), // 15, right-top-front

	// Back
	vec2(1.0f, 0.0f), // 16, left-bottom-back
	vec2(1.0f, 1.0f), // 17, left-top-back
	vec2(0.0f, 0.0f), // 18, right-bottom-back
	vec2(0.0f, 1.0f), // 19, right-top-back

	// Front
	vec2(0.0f, 0.0f), // 20, left-bottom-front
	vec2(0.0f, 1.0f), // 21, left-top-front
	vec2(1.0f, 0.0f), // 22, right-bottom-front
	vec2(1.0f, 1.0f)  // 23, right-top-front
};

static const uint32_t CUBE_MATERIALS[] = {
	// Left
	0, 0, 0, 0,
	// Right
	0, 0, 0, 0,
	// Bottom
	0, 0, 0, 0,
	// Top
	0, 0, 0, 0,
	// Back
	0, 0, 0, 0,
	// Front
	0, 0, 0, 0
};

static const uint32_t CUBE_INDICES[] = {
	// Left
	0, 1, 2,
	3, 2, 1,

	// Right
	5, 4, 7,
	6, 7, 4,

	// Bottom
	8, 10, 9,
	11, 9, 10,

	// Top
	13, 15, 12,
	14, 12, 15,

	// Back
	18, 16, 19,
	17, 19, 16,

	// Front
	20, 22, 21,
	23, 21, 22
};

static const uint32_t CUBE_NUM_VERTICES = sizeof(CUBE_POSITIONS) / sizeof(vec3);
static const uint32_t CUBE_NUM_INDICES = sizeof(CUBE_INDICES) / sizeof(uint32_t);

static ph::Mesh createCubeModel(Allocator* allocator) noexcept
{
	// Create mesh from hardcoded values
	ph::Mesh mesh;
	mesh.vertices.create(CUBE_NUM_VERTICES, allocator);
	mesh.vertices.addMany(CUBE_NUM_VERTICES);
	mesh.materialIndices.create(CUBE_NUM_VERTICES, allocator);
	mesh.materialIndices.addMany(CUBE_NUM_VERTICES);
	for (uint32_t i = 0; i < CUBE_NUM_VERTICES; i++) {
		mesh.vertices[i].pos = CUBE_POSITIONS[i];
		mesh.vertices[i].normal = CUBE_NORMALS[i];
		mesh.vertices[i].texcoord = CUBE_TEXCOORDS[i];
		mesh.materialIndices[i] = CUBE_MATERIALS[i];
	}
	mesh.indices.create(CUBE_NUM_INDICES, allocator);
	mesh.indices.add(CUBE_INDICES, CUBE_NUM_INDICES);

	return mesh;
}

// TestbedUpdateable: Constructors & destructors
// ------------------------------------------------------------------------------------------------

TestbedUpdateable::~TestbedUpdateable() noexcept
{
	PH_LOG(LOG_LEVEL_INFO, "PhantasyTestbed", "TestbedUpdateable destructor");
}

// TestbedUpdateable: Overriden methods from GameLoopUpdateable
// ------------------------------------------------------------------------------------------------

void TestbedUpdateable::initialize(Renderer& renderer)
{
	PH_LOG(LOG_LEVEL_INFO, "PhantasyTestbed", "TestbedUpdateable::initialize()");

	if (mInitialized) return;
	mInitialized = true;

	setLoadImageAllocator(getDefaultAllocator());
	//renderer.addDynamicMesh(createCubeModel(getDefaultAllocator()));
	mLevel = loadStaticSceneSponza("", "resources/sponzaPBR/sponzaPBR.obj", mat44::scaling3(0.05f));
	DynArray<ConstImageView> textureViews;
	for (const auto& texture : mLevel.textures) textureViews.add(texture);
	renderer.setTextures(textureViews);
	renderer.setMaterials(mLevel.materials);
	DynArray<ConstMeshView> meshViews;
	for (const auto& mesh : mLevel.meshes) meshViews.add(mesh);
	renderer.setDynamicMeshes(meshViews);

	// Create RenderEntitites to render
	mEntities.create(mLevel.meshes.size());
	for (uint32_t i = 0; i < mLevel.meshes.size(); i++) {
		ph::RenderEntity entity;
		entity.meshIndex = i;
		mEntities.add(entity);
	}

	// Initial camera
	mCam.pos = vec3(3.0f, 3.0f, 3.0f);
	mCam.dir = sfz::normalize(vec3(-1.0f, -0.25f, -1.0f));
	mCam.up =  vec3(0.0f, 1.0f, 0.0f);
	mCam.near = 0.05f;
	mCam.far = 200.0f;
	mCam.vertFovDeg = 60.0f;

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

		mDynamicSphereLights.add(tmp);
	}
}

UpdateOp TestbedUpdateable::processInput(
	const UserInput& input,
	const UpdateInfo& updateInfo,
	Renderer& renderer)
{
	(void)updateInfo;

	// Update gamecontroller
	updateEmulatedController(input.events, input.rawMouse);
	uint32_t controllerIndex = 0;
	const GameController* controller = input.controllers.get(controllerIndex);
	mCtrl = (controller != nullptr) ? controller->state() : mEmulatedController.state;

	// Update imgui
	updateImgui(renderer, &input.rawMouse, &input.events, &mCtrl);

	return UpdateOp::NO_OP();
}

UpdateOp TestbedUpdateable::updateTick(const UpdateInfo& updateInfo)
{
	float delta = updateInfo.tickTimeSeconds;

	float currentSpeed = 10.0f;
	float turningSpeed = 0.8f * PI;

	// Triggers
	if (mCtrl.leftTrigger > mCtrl.triggerDeadzone) {
		currentSpeed += (mCtrl.leftTrigger * 25.0f);
	}
	if (mCtrl.rightTrigger > mCtrl.triggerDeadzone) {

	}

	// Analogue Sticks
	if (length(mCtrl.rightStick) > mCtrl.stickDeadzone) {
		vec3 right = normalize(cross(mCam.dir, mCam.up));
		mat3 xTurn = mat3::rotation3(vec3(0.0f, -1.0f, 0.0f), mCtrl.rightStick[0] * turningSpeed * delta);
		mat3 yTurn = mat3::rotation3(right, mCtrl.rightStick[1] * turningSpeed * delta);
		setDir(yTurn * xTurn * mCam.dir, yTurn * xTurn * mCam.up);
	}
	if (length(mCtrl.leftStick) > mCtrl.stickDeadzone) {
		vec3 right = normalize(cross(mCam.dir, mCam.up));
		mCam.pos += ((mCam.dir * mCtrl.leftStick[1] + right * mCtrl.leftStick[0]) * currentSpeed * delta);
	}

	// Control Pad
	if (mCtrl.padUp == ButtonState::DOWN) {

	} else if (mCtrl.padDown == ButtonState::DOWN) {

	} else if (mCtrl.padLeft == ButtonState::DOWN) {

	} else if (mCtrl.padRight == ButtonState::DOWN) {

	}

	// Shoulder buttons
	if (mCtrl.leftShoulder == ButtonState::DOWN || mCtrl.leftShoulder == ButtonState::HELD) {
		mCam.pos -= vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
	} else if (mCtrl.rightShoulder == ButtonState::DOWN || mCtrl.rightShoulder == ButtonState::HELD) {
		mCam.pos += vec3(0.0f, 1.0f, 0.0f) * currentSpeed * delta;
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
		return UpdateOp::QUIT();
	}

	setDir(mCam.dir, vec3(0.0f, 1.0f, 0.0f));

	return UpdateOp::NO_OP();
}

void TestbedUpdateable::render(const UpdateInfo& updateInfo, Renderer& renderer)
{
	(void)updateInfo;

	renderer.beginFrame(mCam, mDynamicSphereLights);

	renderer.render(mEntities.data(), mEntities.size());

	// Get Global Config sections
	GlobalConfig& cfg = GlobalConfig::instance();
	mCfgSections.clear();
	cfg.getSections(mCfgSections);
	
	// Start of Imgui commands
	ImGui::NewFrame();

	ImGui::ShowTestWindow();

	// Global Config Window
	ImGui::Begin("Config");
	//ImGui::SetWindowSize("Config", ImVec2(250.0f, 600.0f));
	for (auto& sectionKey : mCfgSections) {

		// Get settings from Global Config
		mCfgSectionSettings.clear();
		cfg.getSectionSettings(sectionKey.str, mCfgSectionSettings);

		// Skip if header is closed
		if (!ImGui::CollapsingHeader(sectionKey.str)) continue;

		for (Setting* setting : mCfgSectionSettings) {
			switch (setting->type()) {
			case VALUE_TYPE_INT:
				ImGui::InputInt(setting->key().str, &setting->value.i.value);
				break;
			case VALUE_TYPE_FLOAT:
				ImGui::InputFloat(setting->key().str, &setting->value.f.value);
				break;
			}
		}
		
	}
	ImGui::End();

	// Render Imgui
	ImGui::Render();
	convertImguiDrawData(mImguiVertices, mImguiIndices, mImguiCommands);
	renderer.renderImgui(mImguiVertices, mImguiIndices, mImguiCommands);
	
	// Finish rendering frame
	renderer.finishFrame();
}

// TestbedUpdateable: Private methods
// ------------------------------------------------------------------------------------------------

void TestbedUpdateable::setDir(vec3 direction, vec3 up) noexcept
{
	mCam.dir = normalize(direction);
	mCam.up = normalize(up - dot(up, mCam.dir) * mCam.dir);
	//sfz_assert_debug(approxEqual(dot(mCam.dir, mCam.up), 0.0f));
}

void TestbedUpdateable::updateEmulatedController(
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
