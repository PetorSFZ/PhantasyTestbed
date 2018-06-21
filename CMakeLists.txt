cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project("Phantasy Testbed" CXX)

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Compiler flags
# ------------------------------------------------------------------------------------------------

if(MSVC)
	# Visual Studio flags
	# /W4 = Warning level 4 (/Wall is too picky and has annoying warnings in standard headers)
	# /wd4201 = Disable warning 4201 (nonstandard extension used : nameless struct/union)
	# /Zi = Produce .pdb debug information. Does not affect optimizations, but does imply /debug.
	# /EHsc = TODO: Add explanation
	# /arch:AVX = Enable (require) Intel AVX instructions for code generation
	# /D_CRT_SECURE_NO_WARNINGS = Removes annyoing warning when using c standard library
	# /utf-8 = Specifies that both the source and execution character sets are encoded using UTF-8.
	set(CMAKE_CXX_FLAGS "/W4 /wd4201 /Zi /EHsc /arch:AVX /D_CRT_SECURE_NO_WARNINGS /utf-8")
	# /O2 = Optimize code for fastest speed
	# /fp:fast = "optimize floating-point code for speed at the expense of accuracy and correctness"
	# /DSFZ_NO_DEBUG = defines the "SFZ_NO_DEBUG" macro, which disables sfz_assert_debug()
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /DSFZ_NO_DEBUG")
	# /DEBUG = "creates debugging information for the .exe file or DLL"
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /fp:fast /DEBUG /DSFZ_NO_DEBUG")
	# /Od = "disables optimization, speeding compilation and simplifying debugging"
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")

elseif(EMSCRIPTEN)
	# Emscripten flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++14 = Enable C++14 support
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	# -s USE_SDL=2 = Use SDL2 library
	# -s TOTAL_MEMORY=1073741824 = 1GiB heap, change this value if you need more
	# -DPH_STATIC_LINK_RENDERER = Link renderer statically instead of dynamically
	# --preload-file resources = Load "resources" directory into generated javascript
	# -DSFZ_NO_DEBUG = Used by sfzCore to disable assertions and such on release builds
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-strict-aliasing -s USE_SDL=2 -s TOTAL_MEMORY=1073741824 -s WASM=0 -s DEMANGLE_SUPPORT=1 -DPH_STATIC_LINK_RENDERER --preload-file resources")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	# macOS flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++14 = Enable C++14 support
	# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	# -DSFZ_NO_DEBUG = Used by sfzCore to disable assertions and such on release builds
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -march=sandybridge -fno-strict-aliasing")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

else()
	message(FATAL_ERROR "Not implemented")
endif()

# Directories
# ------------------------------------------------------------------------------------------------

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)

# Dependencies
# ------------------------------------------------------------------------------------------------

# Make all projects compile to the same directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Include DownloadProject for acquiring dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadProject.cmake)

# SDL2
# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES} and ${SDL2_RUNTIME_FILES}
message("Acquiring SDL2")
download_project(
	PROJ                sdl2
	PREFIX              externals
	GIT_REPOSITORY      https://github.com/PhantasyEngine/Dependency-SDL2.git
	GIT_TAG             b401be45b5caf537411e9a25acc86f6e00677959
	UPDATE_DISCONNECTED 1
	QUIET
)
add_subdirectory(${sdl2_SOURCE_DIR} ${CMAKE_BINARY_DIR}/sdl2)
message("Finished acquiring SDL2")

# stb libraries
# ${STB_FOUND}, ${STB_INCLUDE_DIRS}
message("Acquiring stb libraries")
download_project(
	PROJ                stb
	PREFIX              externals
	GIT_REPOSITORY      https://github.com/PhantasyEngine/Dependency-stb.git
	GIT_TAG             02a690a8110f007ff2edbf4e9571cdd7bfc8e9cb
	UPDATE_DISCONNECTED 1
	QUIET
)
add_subdirectory(${stb_SOURCE_DIR} ${CMAKE_BINARY_DIR}/stb)
message("Finished acquiring stb libraries")

# sfzCore
# ${SFZ_CORE_FOUND}, ${SFZ_CORE_INCLUDE_DIRS}, ${SFZ_CORE_LIBRARIES}
#message("Acquiring sfzCore")
#download_project(
#	PROJ                sfzCore
#	PREFIX              externals
#	GIT_REPOSITORY      https://github.com/PetorSFZ/sfzCore.git
#	GIT_TAG             380611a616f0ff1d9fb241b82fc401087f1a61ba
#	UPDATE_DISCONNECTED 1
#	QUIET
#)
#add_subdirectory(${sfzCore_SOURCE_DIR})
#message("Finished acquiring sfzCore")
add_subdirectory(
	${ROOT_DIR}/sfzCore
	${CMAKE_BINARY_DIR}/sfzCore
)

# dear imgui
# ${IMGUI_FOUND}, ${IMGUI_INCLUDE_DIRS} and ${IMGUI_LIBRARIES}
message("Acquiring dear imgui")
download_project(
	PROJ                imgui
	PREFIX              externals
	GIT_REPOSITORY      https://github.com/PhantasyEngine/Dependency-imgui.git
	GIT_TAG             f130912e438469fdbe3a2f2fb2d9b07574092559
	UPDATE_DISCONNECTED 1
	QUIET
)
add_subdirectory(${imgui_SOURCE_DIR} ${CMAKE_BINARY_DIR}/imgui)
message("Finished acquiring dear imgui")
#add_subdirectory(
#	${ROOT_DIR}/Dependency-imgui
#	${CMAKE_BINARY_DIR}/Dependency-imgui
#)

# sfzGL
add_subdirectory(
	${ROOT_DIR}/sfzGL
	${CMAKE_BINARY_DIR}/sfzGL
)

# Phantasy Engine - Shared Interface
add_subdirectory(
	${ROOT_DIR}/PhantasyEngine-SharedInterface
	${CMAKE_BINARY_DIR}/PhantasyEngine-SharedInterface
)

# Phantasy Engine
# ${PHANTASY_ENGINE_FOUND}, ${PHANTASY_ENGINE_INCLUDE_DIRS} and ${PHANTASY_ENGINE_LIBRARIES}
#message("Acquiring Phantasy Engine")
#download_project(
#	PROJ                PhantasyEngine
#	PREFIX              externals
#	URL                 ${CMAKE_CURRENT_SOURCE_DIR}/../PhantasyEngine
#	GIT_REPOSITORY      https://github.com/PhantasyEngine/phCore.git
#	GIT_TAG             22afd827fcedc880e4597a2df8fc8180c8349d8f
#	UPDATE_DISCONNECTED 1
#	QUIET
#)
#add_subdirectory(${PhantasyEngine_SOURCE_DIR})
#message("Finished acquiring Phantasy Engine")
add_subdirectory(
	${ROOT_DIR}/PhantasyEngine-Core
	${CMAKE_BINARY_DIR}/PhantasyEngine-Core
)

# Renderers, different depending on target platform
if(EMSCRIPTEN)
	# CompatibleGL renderer when building for emscripten
	set(PH_RENDERER_COMPATIBLE_GL_STATIC true)
	add_subdirectory(
		${ROOT_DIR}/Renderer-CompatibleGL
		${CMAKE_BINARY_DIR}/Renderer-CompatibleGL
	)
else()
	# Dynamically linked CompatibleGL renderer for Windows and macOS
	add_subdirectory(
		${ROOT_DIR}/Renderer-CompatibleGL
		${CMAKE_BINARY_DIR}/Renderer-CompatibleGL
	)
endif()

# tinygltf
# ${TINYGLTF_FOUND}, ${TINYGLTF_INCLUDE_DIRS}
add_subdirectory(${EXTERNALS_DIR}/tinygltf)

# Executable
# ------------------------------------------------------------------------------------------------

set(SRC_FILES
	${INCLUDE_DIR}/Cube.hpp
	${INCLUDE_DIR}/GltfLoader.hpp
		${SRC_DIR}/GltfLoader.cpp
	${INCLUDE_DIR}/GltfWriter.hpp
		${SRC_DIR}/GltfWriter.cpp
		${SRC_DIR}/PhantasyTestbedMain.cpp
	${INCLUDE_DIR}/TestbedLogic.hpp
		${SRC_DIR}/TestbedLogic.cpp
)

add_executable(PhantasyTestbed ${SRC_FILES})

target_include_directories(PhantasyTestbed PUBLIC
	${SDL2_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
	${IMGUI_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${PH_SHARED_INTERFACE_INCLUDE_DIRS}
	${PHANTASY_ENGINE_INCLUDE_DIRS}
	${ASSIMP_INCLUDE_DIRS}
	${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(PhantasyTestbed
	${SDL2_LIBRARIES}
	${IMGUI_LIBRARIES}
	${SFZ_CORE_LIBRARIES}
	${PH_SHARED_INTERFACE_LIBRARIES}
	${PHANTASY_ENGINE_LIBRARIES}
	${ASSIMP_LIBRARIES}
)

if(EMSCRIPTEN)
	target_link_libraries(PhantasyTestbed ${PH_RENDERER_COMPATIBLE_GL_LIBRARIES})
else()
	# Ensure the CompatibleGL Renderer is built even though it is not explicitly linked to the exe
	add_dependencies(PhantasyTestbed ${PH_RENDERER_COMPATIBLE_GL_LIBRARIES})
endif()

# Copy runtime files
# ------------------------------------------------------------------------------------------------

if(MSVC)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)

	file(COPY ${SFZ_GL_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${SFZ_GL_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${SFZ_GL_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)

	file(COPY ${ASSIMP_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${ASSIMP_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${ASSIMP_DLLS} DESTINATION ${CMAKE_BINARY_DIR}/Release)
endif()
