// See 'LICENSE_PHANTASY_ENGINE' for copyright and contributors.

#pragma once

#include <sfz/strings/StringHashers.hpp>
#include <sfz/containers/DynArray.hpp>
#include <sfz/containers/HashMap.hpp>
#include <sfz/math/Matrix.hpp>
#include <sfz/math/Vector.hpp>
#include <sfz/strings/StackString.hpp>

#include <ph/rendering/Image.hpp>
#include <ph/rendering/Material.h>
#include <ph/rendering/Mesh.h>
#include <ph/rendering/Vertex.h>

namespace ph {

using sfz::DynArray;
using sfz::mat4;
using sfz::StackString256;
using sfz::HashMap;

struct Level final {
	DynArray<Mesh> meshes;
	DynArray<Image> textures;
	DynArray<Material> materials;

	HashMap<StackString256, uint32_t> texMapping;
};

// Sponza loading functions
// ------------------------------------------------------------------------------------------------

Level loadStaticSceneSponza(
	const char* basePath,
	const char* fileName,
	const mat4& modelMatrix = mat4::identity()) noexcept;

} // namespace ph
