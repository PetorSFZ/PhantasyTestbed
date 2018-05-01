#pragma once

#include "ph/rendering/LevelAssets.hpp"

namespace ph {

/// Loads all meshes, textures and materials from a .gltf file into the level assets.
/// All scene graph information, relative positions and transformations are ignored.
bool loadAssetsFromGltf(
	const char* gltfPath,
	LevelAssets& assets) noexcept;

} // namespace ph
