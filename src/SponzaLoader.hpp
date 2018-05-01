// See 'LICENSE_PHANTASY_ENGINE' for copyright and contributors.

#pragma once

#include <sfz/math/Matrix.hpp>

#include "ph/rendering/LevelAssets.hpp"

namespace ph {

using sfz::mat4;

// Sponza loading functions
// ------------------------------------------------------------------------------------------------

bool loadStaticSceneSponza(
	const char* basePath,
	const char* fileName,
	LevelAssets& assets,
	const mat4& modelMatrix = mat4::identity()) noexcept;

} // namespace ph
