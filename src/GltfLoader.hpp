#pragma once

#include "ph/rendering/Mesh.hpp"

namespace ph {

Mesh loadMeshFromGltf(const char* basePath, const char* gltfPath, uint32_t tmpMatIdx) noexcept;

} // namespace ph
