cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project("Phantasy Testbed" LANGUAGES CXX)

# Generate a "compile_commands.json" for VSCode and such when compiling with make
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Phantasy Engine
# ------------------------------------------------------------------------------------------------

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(PHANTASY_ENGINE_DIR ${ROOT_DIR}/PhantasyEngine)

include(${PHANTASY_ENGINE_DIR}/PhantasyEngine.cmake)

phSetCompilerFlags()
phPrintCompilerFlags()

# Temp
# ------------------------------------------------------------------------------------------------

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(EXTERNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)

# Include DownloadProject for acquiring dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadProject.cmake)

# SDL2
# ${SDL2_FOUND}, ${SDL2_INCLUDE_DIRS}, ${SDL2_LIBRARIES} and ${SDL2_RUNTIME_FILES}
set(PH_SDL2_ROOT ${ROOT_DIR}/Dependency-SDL2)
phAddSDL2()

# sfzCore
# ${SFZ_CORE_FOUND}, ${SFZ_CORE_INCLUDE_DIRS}, ${SFZ_CORE_LIBRARIES}
# ${SFZ_CORE_OPENGL_FOUND}, ${SFZ_CORE_OPENGL_INCLUDE_DIRS}, ${SFZ_CORE_OPENGL_LIBRARIES}, ${SFZ_CORE_OPENGL_RUNTIME_FILES}
set(PH_SFZ_CORE_ROOT ${ROOT_DIR}/sfzCore)
set(SFZ_CORE_OPENGL true)
phAddSfzCore()

# stb: ${STB_FOUND}, ${STB_INCLUDE_DIRS}
# dear-imgui: ${IMGUI_FOUND}, ${IMGUI_INCLUDE_DIRS}, ${IMGUI_LIBRARIES}
phAddBundledExternals()

phAddPhantasyEngineTargets()

phAddRendererCompatibleGL()

# tinygltf
# ${TINYGLTF_FOUND}, ${TINYGLTF_INCLUDE_DIRS}
add_subdirectory(${EXTERNALS_DIR}/tinygltf)

# Executable
# ------------------------------------------------------------------------------------------------

set(SRC_FILES
	${SRC_DIR}/Cube.hpp
	${SRC_DIR}/GltfLoader.hpp
	${SRC_DIR}/GltfLoader.cpp
	${SRC_DIR}/GltfWriter.hpp
	${SRC_DIR}/GltfWriter.cpp
	${SRC_DIR}/PhantasyTestbedMain.cpp
	${SRC_DIR}/TestbedLogic.hpp
	${SRC_DIR}/TestbedLogic.cpp
)
source_group(TREE ${SRC_DIR} FILES ${SRC_FILES})

add_executable(PhantasyTestbed ${SRC_FILES})

target_include_directories(PhantasyTestbed PUBLIC
	${SRC_DIR}
	${TINYGLTF_INCLUDE_DIRS}
)

phLinkSDL2(PhantasyTestbed)
phLinkSfzCore(PhantasyTestbed)
phLinkBundledExternals(PhantasyTestbed)
phLinkPhantasyEngine(PhantasyTestbed)
phLinkRendererCompatibleGL(PhantasyTestbed)

# Copy runtime files
# ------------------------------------------------------------------------------------------------

if(MSVC)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${SDL2_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)

	file(COPY ${SFZ_CORE_OPENGL_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${SFZ_CORE_OPENGL_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${SFZ_CORE_OPENGL_RUNTIME_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release)
endif()

# Create symlink script
# ------------------------------------------------------------------------------------------------

phCreateSymlinkScript(${RESOURCES_DIR} ${PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR})

# iOS specific stuff
# ------------------------------------------------------------------------------------------------

if (IOS)
	target_link_libraries(PhantasyTestbed "-framework AudioToolbox")
	target_link_libraries(PhantasyTestbed "-framework AVFoundation")
	target_link_libraries(PhantasyTestbed "-framework CoreGraphics")
	target_link_libraries(PhantasyTestbed "-framework CoreMotion")
	target_link_libraries(PhantasyTestbed "-framework Foundation")
	target_link_libraries(PhantasyTestbed "-framework GameController")
	target_link_libraries(PhantasyTestbed "-framework OpenGLES")
	target_link_libraries(PhantasyTestbed "-framework QuartzCore")
	target_link_libraries(PhantasyTestbed "-framework UIKit")
	target_link_libraries(PhantasyTestbed "-framework Metal")

	# Copy res directory into application bundle
	add_custom_command(TARGET PhantasyTestbed POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${RESOURCES_DIR} $<TARGET_FILE_DIR:PhantasyTestbed>/${RESOURCES_DIR_NAME}
	)

	# Copy Renderer-CompatibleGL's res dir into application bundle
	get_filename_component(COMP_GL_RES_DIR_NAME ${PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR} NAME)
	add_custom_command(TARGET PhantasyTestbed POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PH_RENDERER_COMPATIBLE_GL_RUNTIME_DIR} $<TARGET_FILE_DIR:PhantasyTestbed>/${COMP_GL_RES_DIR_NAME}
	)
endif()
