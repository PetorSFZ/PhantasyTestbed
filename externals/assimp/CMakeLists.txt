if(MSVC14)
	# Return pre-built DLL version to parent for linking
	set(ASSIMP_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/lib/win64/assimp-vc140-mt.lib PARENT_SCOPE)
	set(ASSIMP_DLLS ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/assimp-vc140-mt.dll PARENT_SCOPE)
else()
	# Assimp flags
	set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "." FORCE)
	set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "." FORCE)
	set(ASSIMP_BUILD_STATIC_LIB ON CACHE BOOL "." FORCE)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "." FORCE)
	set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "." FORCE)

	# Build Assimp
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp-3.3.1)

	# Compile options (fix warnings)
	if(MSVC)
		target_compile_options(assimp PUBLIC /W1)
	else()
		target_compile_options(assimp PUBLIC -w)
	endif()

	# Return library to parent for linking
	set(ASSIMP_LIBRARIES assimp PARENT_SCOPE)
endif()

# Return include directory to parent
set(ASSIMP_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/assimp-3.3.1/include PARENT_SCOPE)
