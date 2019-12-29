cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project("Phantasy Testbed" LANGUAGES CXX)

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Phantasy Engine initialization
# ------------------------------------------------------------------------------------------------

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SFZ_TECH_DIR ${ROOT_DIR}/sfz_tech)

include(${SFZ_TECH_DIR}/PhantasyEngine.cmake)

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

# ZeroG
# ${ZEROG_FOUND}, ${ZEROG_INCLUDE_DIRS}, ${ZEROG_LIBRARIES} and ${ZEROG_RUNTIME_FILES}
# ${ZEROG_CPP_FOUND}, ${ZEROG_CPP_INCLUDE_DIRS} and ${ZEROG_CPP_LIBRARIES}
#set(PH_ZEROG_ROOT ${ROOT_DIR}/ZeroG)
phAddZeroG()

# stb: ${STB_FOUND}, ${STB_INCLUDE_DIRS}
# dear-imgui: ${IMGUI_FOUND}, ${IMGUI_INCLUDE_DIRS}, ${IMGUI_LIBRARIES}
# tinygltf: ${TINYGLTF_FOUND}, ${TINYGLTF_INCLUDE_DIRS}
phAddBundledExternals()

# PhantasyEngine
# ${PHANTASY_ENGINE_FOUND}, ${PHANTASY_ENGINE_INCLUDE_DIRS}, ${PHANTASY_ENGINE_LIBRARIES}
phAddPhantasyEngineTargets()

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
phIosLinkStandardFrameworks(PhantasyTestbed)

# Files
# ------------------------------------------------------------------------------------------------

phMsvcCopyRuntimeDLLs(${SDL2_RUNTIME_FILES} ${ZEROG_RUNTIME_FILES})

phCreateSymlinkScript(${RESOURCES_DIR} ${PHANTASY_ENGINE_RUNTIME_DIR})

phExecuteSymlinkScript()

phIosCopyRuntimeDirectories(PhantasyTestbed ${RESOURCES_DIR} ${PHANTASY_ENGINE_RUNTIME_DIR})
