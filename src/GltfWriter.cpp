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

using sfz::DynString;
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

static str96 getFileName(const char* path) noexcept
{
	str96 tmp;
	int32_t len = int32_t(strlen(path));
	for (int32_t i = len; i > 0; i--) {
		char c = path[i-1];
		if (c == '\\' || c == '/') {
			tmp.printf("%s", path + i);
			break;
		}
	}
	return tmp;
}

static str96 stripFileEnding(const str96& fileName) noexcept
{
	str96 tmp = fileName;

	for (int32_t i = int32_t(fileName.size()) - 1; i >= 0; i--) {
		const char c = fileName.str[i];
		if (c == '.') {
			tmp.str[i] = '\0';
			break;
		}
	}

	return tmp;
}

// Write gltf asset header (non-optional)
// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#asset
static void writeHeader(DynString& gltf) noexcept
{
	gltf.printfAppend("%s", "{\n");
	gltf.printfAppend("%s", "\t\"asset\": {\n");
	gltf.printfAppend("%s", "\t\t\"version\": \"2.0\",\n");
	gltf.printfAppend("%s", "\t\t\"generator\": \"Phantasy Engine Exporter v1.0\"\n");
	gltf.printfAppend("%s", "\t},\n");
}

struct MeshComponent {
	DynArray<uint32_t> indices;
	uint32_t materialIdx;
};

// Sorts all triangles in a mesh into different components where each component uses only one
// material. If the entire mesh uses a single material only one component will be returned.
static DynArray<MeshComponent> componentsFromMesh(const ConstMeshView& mesh) noexcept
{
	sfz_assert_debug((mesh.numIndices % 3) == 0);

	DynArray<MeshComponent> components;
	components.create(10);

	for (uint32_t i = 0; i < mesh.numIndices; i += 3) {
		uint32_t idx0 = mesh.indices[i + 0];
		uint32_t idx1 = mesh.indices[i + 1];
		uint32_t idx2 = mesh.indices[i + 2];

		// Require material to be same for entire triangle
		uint32_t m0 = mesh.materialIndices[idx0];
		uint32_t m1 = mesh.materialIndices[idx1];
		uint32_t m2 = mesh.materialIndices[idx2];
		sfz_assert_debug(m0 == m1);
		sfz_assert_debug(m1 == m2);

		// Try to find existing component with same material index
		DynArray<uint32_t>* indicesPtr = nullptr;
		for (MeshComponent& component : components) {
			if (component.materialIdx == m0) {
				indicesPtr = &component.indices;
			}
		}

		// If component did not exist, create it
		if (indicesPtr == nullptr) {
			components.add(MeshComponent());
			components.last().materialIdx = m0;
			indicesPtr = &components.last().indices;
			indicesPtr->create(mesh.numIndices);
		}

		// Add indicies to component
		indicesPtr->add(idx0);
		indicesPtr->add(idx1);
		indicesPtr->add(idx2);
	}

	return components;
}

static void writeMaterials(DynString& gltf, const DynArray<Material>& materials) noexcept
{
	gltf.printfAppend("%s", "\t\"materials\": [\n");

	auto u8tof32 = [](uint8_t val) {
		return float(val) * (1.0f / 255.0f);
	};

	for (uint32_t i = 0; i < materials.size(); i++) {
		const Material& m = materials[i];

		gltf.printfAppend("%s", "\t\t{\n");
	
		// Name
		gltf.printfAppend("\t\t\t\"name\": \"%s\",\n", "UnknownMaterialName");

		// PBR material
		gltf.printfAppend("%s", "\t\t\t\"pbrMetallicRoughness\": {\n");

		// Albedo
		gltf.printfAppend("\t\t\t\t\"baseColorFactor\": [%.4f, %.4f, %.4f, %.4f],\n",
			u8tof32(m.albedo.x),
			u8tof32(m.albedo.y),
			u8tof32(m.albedo.z),
			u8tof32(m.albedo.w));

		// Albedo texture
		if (m.albedoTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\t\"baseColorTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\t\"index\": %u\n", uint32_t(m.albedoTexIndex));
			gltf.printfAppend("%s", "\t\t\t\t},\n");
		}

		// Roughness
		gltf.printfAppend("\t\t\t\t\"roughnessFactor\": %.4f,\n", u8tof32(m.roughness));

		// Metallic
		gltf.printfAppend("\t\t\t\t\"metallicFactor\": %.4f", u8tof32(m.metallic));

		// Metallic roughness texture
		if (m.metallicRoughnessTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", ",\n");

			gltf.printfAppend("%s", "\t\t\t\t\"metallicRoughnessTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\t\"index\": %u\n", uint32_t(m.metallicRoughnessTexIndex));
			gltf.printfAppend("%s", "\t\t\t\t}\n");
		}
		else {
			gltf.printfAppend("%s", "\n");
		}

		// End PBR material
		gltf.printfAppend("%s", "\t\t\t},\n");

		// Normal texture
		if (m.normalTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\"normalTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\"index\": %u\n", uint32_t(m.normalTexIndex));
			gltf.printfAppend("%s", "\t\t\t},\n");
		}

		// Oclussion texture
		if (m.occlusionTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\"occlusionTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\"index\": %u\n", uint32_t(m.occlusionTexIndex));
			gltf.printfAppend("%s", "\t\t\t},\n");
		}

		// Emissive texture
		if (m.emissiveTexIndex != uint16_t(~0)) {
			gltf.printfAppend("%s", "\t\t\t\"emissiveTexture\": {\n");
			gltf.printfAppend("\t\t\t\t\"index\": %u\n", uint32_t(m.emissiveTexIndex));
			gltf.printfAppend("%s", "\t\t\t},\n");
		}

		// Emissive
		gltf.printfAppend("\t\t\t\"emissiveFactor\": [%.4f, %.4f, %.4f]\n",
			u8tof32(m.emissive.x),
			u8tof32(m.emissive.y),
			u8tof32(m.emissive.z));

		if ((i + 1) == materials.size()) gltf.printfAppend("%s", "\t\t}\n");
		else gltf.printfAppend("%s", "\t\t},\n");
	}

	gltf.printfAppend("%s", "\t],\n");
}

static void writeTextures(
	DynString& gltf,
	const char* basePath,
	const LevelAssets& assets,
	const DynArray<uint32_t>& texIndices) noexcept
{
	sfz_assert_debug(assets.textures.size() == assets.textureFileMappings.size());
	if (texIndices.size() == 0) return;

	// Attempt to create directory for textures if necessary
	sfz::createDirectory(str320("%stextures", basePath));

	// Write "images" section
	gltf.printfAppend("%s", "\t\"images\": [\n");
	for (uint32_t i = 0; i < texIndices.size(); i++) {
		uint32_t originalTexIndex = texIndices[i];
		const FileMapping& mapping = assets.textureFileMappings[originalTexIndex];
		str96 fileNameWithoutEnding = stripFileEnding(mapping.fileName);

		// Write image to file
		str320 imageWritePath("%stextures/%s.png", basePath, fileNameWithoutEnding.str);
		if (!saveImagePng(assets.textures[originalTexIndex], imageWritePath)) {
			SFZ_ERROR("glTF writer", "Failed to write image \"%s\" to path \"%s\"",
				fileNameWithoutEnding.str, imageWritePath.str);
		}

		// Write uri to gltf string
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("\t\t\t\"uri\": \"textures/%s.png\"\n", fileNameWithoutEnding.str);
		if ((i + 1) == texIndices.size()) gltf.printfAppend("%s", "\t\t}\n");
		else gltf.printfAppend("%s", "\t\t},\n");
	}
	gltf.printfAppend("%s", "\t],\n");

	// Write "textures" section
	gltf.printfAppend("%s", "\t\"textures\": [\n");
	for (uint32_t i = 0; i < texIndices.size(); i++) {
		gltf.printfAppend("%s", "\t\t{\n");
		gltf.printfAppend("\t\t\t\"source\": %u\n", i);
		if ((i + 1) == texIndices.size()) gltf.printfAppend("%s", "\t\t}\n");
		else gltf.printfAppend("%s", "\t\t},\n");
	}
	gltf.printfAppend("%s", "\t],\n");
}

static void writeExit(DynString& gltf) noexcept
{
	gltf.printfAppend("%s", "}\n");
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

	// Get file name
	str96 fileNameWithoutEnding = stripFileEnding(getFileName(writePath));

	// Calculate how many bytes will be needed for writing the binary data
	uint32_t numBytesCombinedBinaryData = 0;
	for (uint32_t i = 0; i < meshIndices.size(); i++) {
		uint32_t meshIdx = meshIndices[i];
		const Mesh& mesh = assets.meshes[meshIdx];
		numBytesCombinedBinaryData += mesh.vertices.size() * sizeof(Vertex);
		numBytesCombinedBinaryData += mesh.indices.size() * sizeof(uint32_t);
	}

	// Allocate memory for writing (plus some extra)
	DynArray<uint8_t> combinedBinaryData(numBytesCombinedBinaryData + 8192);

	// Offsets into combined binary data array
	struct MeshOffsets {
		uint32_t vertexOffset;
		DynArray<uint32_t> indicesOffsets;
	};
	DynArray<MeshOffsets> binaryOffsets(meshIndices.size());

	// Create gltf string to fill in
	const uint32_t GLTF_MAX_CAPACITY = 10 * 1024 * 1024; // Assume max 10 MiB for the .gltf file
	sfz::DynString tempGltfString("", GLTF_MAX_CAPACITY);

	writeHeader(tempGltfString);

	// Information about which materials to write
	DynArray<Material> materialsToWrite;
	DynArray<uint32_t> materialsOriginalIndex;
	materialsToWrite.create(100);
	materialsOriginalIndex.create(100);
	
	// Go through all meshes, split into components and combine into combined binary data
	for (uint32_t i = 0; i < meshIndices.size(); i++) {
		uint32_t meshIdx = meshIndices[i];

		// Print error message and skip if mesh does not exist
		if (meshIdx >= assets.meshes.size()) {
			SFZ_ERROR("glTF writer", "Trying to write mesh that does not exist: %u", meshIdx);
			continue;
		}

		// Split mesh into components
		const Mesh& mesh = assets.meshes[meshIdx];
		DynArray<MeshComponent> components = componentsFromMesh(mesh);

		// Add new materials from components materials to write list and modify component to use
		// the new indices
		for (MeshComponent& component : components) {
			
			// Linear search through materials to write
			bool materialFound = false;
			for (uint32_t j = 0; j < materialsOriginalIndex.size(); j++) {
				if (materialsOriginalIndex[j] != component.materialIdx) continue;
				component.materialIdx = j;
				materialFound = true;
				break;
			}

			// Add material if not in list
			if (!materialFound) {
				materialsToWrite.add(assets.materials[component.materialIdx]);
				materialsOriginalIndex.add(component.materialIdx);
				component.materialIdx = materialsToWrite.size() - 1;
			}
		}

		// Add vertex data to combined binary data
		MeshOffsets offsets;
		offsets.vertexOffset = combinedBinaryData.size();
		combinedBinaryData.add(reinterpret_cast<const uint8_t*>(mesh.vertices.data()),
			mesh.vertices.size() * sizeof(Vertex));
		
		// Add indices to combined binary data
		offsets.indicesOffsets.create(components.size());
		for (const MeshComponent& component : components) {
			offsets.indicesOffsets.add(combinedBinaryData.size());
			combinedBinaryData.add(reinterpret_cast<const uint8_t*>(component.indices.data()),
				component.indices.size() * sizeof(uint32_t));
		}
	}

	// Write binary data to file
	bool binaryWriteSuccess = sfz::writeBinaryFile(
		str320("%s%s.bin",basePath.str, fileNameWithoutEnding.str),
		combinedBinaryData.data(),
		combinedBinaryData.size());
	if (!binaryWriteSuccess) {
		SFZ_ERROR("glTF writer", "Failed to write binary data to file: \"%s\"",
			str320("%s%s.bin", basePath.str, fileNameWithoutEnding.str).str);
		return false;
	}

	// Go through materials to write and find all textures to write, also update texture indices in
	// materials to reflect their new indices in the gltf file.
	DynArray<uint32_t> texturesToWrite;
	texturesToWrite.create(100);
	for (uint32_t i = 0; i < materialsToWrite.size(); i++) {
		Material& m = materialsToWrite[i];

		// Albedo
		if (m.albedoTexIndex != uint16_t(~0)) {
			sfz_assert_debug(m.albedoTexIndex < assets.textures.size());
			texturesToWrite.add(m.albedoTexIndex);
			m.albedoTexIndex = texturesToWrite.size() - 1;
		}

		// MetallicRoughness
		if (m.metallicRoughnessTexIndex != uint16_t(~0)) {
			sfz_assert_debug(m.metallicRoughnessTexIndex < assets.textures.size());
			texturesToWrite.add(m.metallicRoughnessTexIndex);
			m.metallicRoughnessTexIndex = texturesToWrite.size() - 1;
		}

		// Normal
		if (m.normalTexIndex != uint16_t(~0)) {
			sfz_assert_debug(m.normalTexIndex < assets.textures.size());
			texturesToWrite.add(m.normalTexIndex);
			m.normalTexIndex = texturesToWrite.size() - 1;
		}

		// Occlusion
		if (m.occlusionTexIndex != uint16_t(~0)) {
			sfz_assert_debug(m.occlusionTexIndex < assets.textures.size());
			texturesToWrite.add(m.occlusionTexIndex);
			m.occlusionTexIndex = texturesToWrite.size() - 1;
		}

		// Emissive
		if (m.emissiveTexIndex != uint16_t(~0)) {
			sfz_assert_debug(m.emissiveTexIndex < assets.textures.size());
			texturesToWrite.add(m.emissiveTexIndex);
			m.emissiveTexIndex = texturesToWrite.size() - 1;
		}
	}

	// Write materials
	writeMaterials(tempGltfString, materialsToWrite);

	// Write textures
	writeTextures(tempGltfString, basePath, assets, texturesToWrite);

	writeExit(tempGltfString);

	bool writeSuccess = sfz::writeTextFile(writePath, tempGltfString.str());
	if (!writeSuccess) {
		SFZ_ERROR("glTF writer", "Failed to write to \"%s\"", writePath);
		return false;
	}


	return false;
}

} // namespace ph
