cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project("Phantasy Testbed")

# Sets build type to release if no build type is specified in a single-configuration generator.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

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

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	# macOS flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++14 = Enable C++14 support
	# -march=sandybridge = Require at least a Sandy Bridge Intel CPU to run code
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	# -DSFZ_NO_DEBUG = Used by sfzCore to disable assertions and such on release builds
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -march=sandybridge -fno-strict-aliasing -DPH_STATIC_LINK_RENDERER")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ffast-math -g -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DSFZ_NO_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

elseif(EMSCRIPTEN)
	# Emscripten flags
	# -Wall -Wextra = Enable most warnings
	# -std=c++14 = Enable C++14 support
	# -fno-strict-aliasing = Disable strict aliasing optimizations
	# -s USE_SDL=2 = Use SDL2 library
	# -s TOTAL_MEMORY=134217728 = 128MiB heap, change this value if you need more
	# -DPH_STATIC_LINK_RENDERER = Link renderer statically instead of dynamically
	# -DSFZ_NO_DEBUG = Used by sfzCore to disable assertions and such on release builds
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-strict-aliasing -s USE_SDL=2 -s TOTAL_MEMORY=134217728 -DPH_STATIC_LINK_RENDERER")
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
	GIT_TAG             4db8582803583c35d8377bc23795bfa0c0cbbc5f
	UPDATE_DISCONNECTED 1
	QUIET
)
add_subdirectory(${sdl2_SOURCE_DIR})
message("Finished acquiring SDL2")

# sfzCore
add_subdirectory(
	${ROOT_DIR}/sfzCore
	${CMAKE_BINARY_DIR}/sfzCore
)

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
	# WebGL renderer when building for emscripten
	add_subdirectory(
		${ROOT_DIR}/Renderer-WebGL
		${CMAKE_BINARY_DIR}/Renderer-WebGL
	)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	# Static linked ModernGL renderer for macOS
	set(PH_RENDERER_MODERN_GL_STATIC true)
	add_subdirectory(
		${ROOT_DIR}/Renderer-ModernGL
		${CMAKE_BINARY_DIR}/Renderer-ModernGL
	)
else()
	# ModernGL renderer when building for Windows
	add_subdirectory(
		${ROOT_DIR}/Renderer-ModernGL
		${CMAKE_BINARY_DIR}/Renderer-ModernGL
	)
endif()

# Executable
# ------------------------------------------------------------------------------------------------

set(SRC_FILES
	${INCLUDE_DIR}/TestbedUpdateable.hpp
	${SRC_DIR}/TestbedUpdateable.cpp
	${SRC_DIR}/PhantasyTestbedMain.cpp
)

add_executable(PhantasyTestbed ${SRC_FILES})

target_include_directories(PhantasyTestbed PUBLIC
	${SDL2_INCLUDE_DIRS}
	${SFZ_CORE_INCLUDE_DIRS}
	${PH_SHARED_INTERFACE_INCLUDE_DIRS}
	${PHANTASY_ENGINE_INCLUDE_DIRS}
)

target_link_libraries(PhantasyTestbed
	${SDL2_LIBRARIES}
	${SFZ_CORE_LIBRARIES}
	${PHANTASY_ENGINE_LIBRARIES}
)

if(EMSCRIPTEN)
	target_link_libraries(PhantasyTestbed ${PH_RENDERER_WEB_GL_LIBRARIES})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	target_link_libraries(PhantasyTestbed ${PH_RENDERER_MODERN_GL_LIBRARIES})
else()
	# Ensure the Modern-GL Renderer is built even though it is not explicitly linked to the exe
	add_dependencies(PhantasyTestbed ${PH_RENDERER_MODERN_GL_LIBRARIES})
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
endif()
