cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project("Phantasy Testbed" LANGUAGES CXX)

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Phantasy Engine initialization
# ------------------------------------------------------------------------------------------------

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(PHANTASY_ENGINE_DIR ${ROOT_DIR}/PhantasyEngine)

include(${PHANTASY_ENGINE_DIR}/PhantasyEngine.cmake)

# Phantasy Engine setup
# ------------------------------------------------------------------------------------------------

phSetCompilerFlags()
phPrintCompilerFlags()

# SDL2
# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES} and ${SDL2_RUNTIME_FILES}
#set(PH_SDL2_ROOT ${ROOT_DIR}/Dependency-SDL2)
phAddSDL2()

# sfzCore
# ${SFZ_CORE_FOUND}, ${SFZ_CORE_INCLUDE_DIRS}, ${SFZ_CORE_LIBRARIES}
# ${SFZ_CORE_OPENGL_FOUND}, ${SFZ_CORE_OPENGL_INCLUDE_DIRS}, ${SFZ_CORE_OPENGL_LIBRARIES}, ${SFZ_CORE_OPENGL_RUNTIME_FILES}
#set(PH_SFZ_CORE_ROOT ${ROOT_DIR}/sfzCore)
set(SFZ_CORE_OPENGL true)
phAddSfzCore()

# stb: ${STB_FOUND}, ${STB_INCLUDE_DIRS}
# dear-imgui: ${IMGUI_FOUND}, ${IMGUI_INCLUDE_DIRS}, ${IMGUI_LIBRARIES}
# tinygltf: ${TINYGLTF_FOUND}, ${TINYGLTF_INCLUDE_DIRS}
phAddBundledExternals()

# Interface: ${PH_INTERFACE_FOUND}, ${PH_INTERFACE_INCLUDE_DIRS}, ${PH_INTERFACE_LIBRARIES}
# Engine: ${PHANTASY_ENGINE_FOUND}, ${PHANTASY_ENGINE_INCLUDE_DIRS}, ${PHANTASY_ENGINE_LIBRARIES}
phAddPhantasyEngineTargets()

# ${PH_RENDERER_COMPATIBLE_GL_FOUND}, ${PH_RENDERER_COMPATIBLE_GL_LIBRARIES}, ${PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR}
phAddRendererCompatibleGL()

# Executable
# ------------------------------------------------------------------------------------------------

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)

set(SRC_FILES
	${SRC_DIR}/Cube.hpp
	${SRC_DIR}/PhantasyTestbedMain.cpp
	${SRC_DIR}/TestbedLogic.hpp
	${SRC_DIR}/TestbedLogic.cpp
)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

add_executable(PhantasyTestbed ${SRC_FILES})

target_include_directories(PhantasyTestbed PUBLIC
	${SRC_DIR}
)

phLinkSDL2(PhantasyTestbed)
phLinkSfzCore(PhantasyTestbed)
phLinkBundledExternals(PhantasyTestbed)
phLinkPhantasyEngine(PhantasyTestbed)
phLinkRendererCompatibleGL(PhantasyTestbed)
phIosLinkStandardFrameworks(PhantasyTestbed)

# Files
# ------------------------------------------------------------------------------------------------

phMsvcCopyRuntimeDLLs(${SDL2_RUNTIME_FILES} ${SFZ_CORE_OPENGL_RUNTIME_FILES})

phCreateSymlinkScript(${RESOURCES_DIR} ${PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR})

phIosCopyRuntimeDirectories(PhantasyTestbed ${RESOURCES_DIR} ${PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR})
