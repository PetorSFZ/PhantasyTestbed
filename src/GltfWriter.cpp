// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "GltfWriter.hpp"

#include <sfz/Logging.hpp>
#include <sfz/strings/StackString.hpp>
#include <sfz/util/IO.hpp>

namespace ph {

using sfz::str320;

// Statics
// ------------------------------------------------------------------------------------------------

static str320 calculateBasePath(const char* path) noexcept
{
	str320 str("%s", path);

	// Go through path until the path separator is found
	bool success = false;
	for (uint32_t i = str.size() - 1; i > 0; i--) {
		const char c = str.str[i - 1];
		if (c == '\\' || c == '/') {
			str.str[i] = '\0';
			success = true;
			break;
		}
	}

	// If no path separator is found, assume we have no base path
	if (!success) str.printf("");

	return str;
}

// Entry function
// ------------------------------------------------------------------------------------------------

bool writeAssetsToGltf(
	const char* writePath,
	LevelAssets& assets,
	const DynArray<uint32_t>& meshIndices) noexcept
{
	// Try to create base directory if it does not exist
	str320 basePath = calculateBasePath(writePath);
	if (!sfz::directoryExists(basePath)) {
		bool success = sfz::createDirectory(basePath);
		if (!success) {
			SFZ_ERROR("glTF writer", "Failed to create directory \"%s\"", basePath.str);
			return false;
		}
	}

	bool writeSuccess = sfz::writeTextFile(writePath, "");
	if (!writeSuccess) {
		SFZ_ERROR("glTF writer", "Failed to write to \"%s\"", writePath);
		return false;
	}


	return false;
}

} // namespace ph
