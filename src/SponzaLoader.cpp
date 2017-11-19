// See 'LICENSE_PHANTASY_ENGINE' for copyright and contributors.

#include "SponzaLoader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <sfz/containers/HashMap.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/strings/DynString.hpp>
#include <sfz/strings/StringHashers.hpp>

#include <ph/utils/Logging.hpp>

namespace ph {

using namespace sfz;

static bool approxEqual(const Material& lhs, const Material& rhs) noexcept
{
	return
		lhs.albedoTexIndex == rhs.albedoTexIndex &&
		lhs.roughnessTexIndex == rhs.roughnessTexIndex &&
		lhs.metallicTexIndex == rhs.metallicTexIndex &&
		sfz::approxEqual(lhs.albedo, rhs.albedo) &&
		sfz::approxEqual(lhs.roughness, rhs.roughness) &&
		sfz::approxEqual(lhs.metallic, rhs.metallic);
}

static vec3 toSFZ(const aiVector3D& v)
{
	return vec3(v.x, v.y, v.z);
}

static void processNode(const char* basePath, Level& level,
                        const aiScene* scene, aiNode* node, const mat4& modelMatrix, const mat4& normalMatrix) noexcept
{
	aiString tmpPath;

	// Process all meshes in current node
	for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; meshIndex++) {

		const aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIndex]];
		Mesh meshTmp;
		Material materialTmp;

		// Allocate memory for vertices
		meshTmp.vertices.addMany(mesh->mNumVertices);

		// Fill vertices with positions, normals and uv coordinates
		sfz_assert_debug(mesh->HasPositions());
		sfz_assert_debug(mesh->HasNormals());
		for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
			Vertex& v = meshTmp.vertices[i];
			v.pos = sfz::transformPoint(modelMatrix, toSFZ(mesh->mVertices[i]));
			v.normal = sfz::transformDir(normalMatrix, toSFZ(mesh->mNormals[i]));
			if (mesh->mTextureCoords[0] != nullptr) {
				v.texcoord = toSFZ(mesh->mTextureCoords[0][i]).xy;
			}
			else {
				v.texcoord = vec2(0.0f);
			}
		}

		// Fill geometry with indices
		meshTmp.indices.setCapacity(mesh->mNumFaces * 3u);
		for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
			const aiFace& face = mesh->mFaces[i];
			meshTmp.indices.add(face.mIndices, face.mNumIndices);
		}

		// Retrieve mesh's material
		const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

		// Albedo (stored in diffuse for sponza pbr)
		if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			sfz_assert_debug(mat->GetTextureCount(aiTextureType_DIFFUSE) == 1);

			tmpPath.Clear();
			mat->GetTexture(aiTextureType_DIFFUSE, 0, &tmpPath);

			const uint32_t* indexPtr = level.texMapping.get(tmpPath.C_Str());
			if (indexPtr == nullptr) {
				PH_LOG(LOG_LEVEL_INFO, "SponzaLoader", "Loaded albedo texture: %s",
					tmpPath.C_Str());

				const uint32_t nextIndex = level.textures.size();
				level.texMapping[tmpPath.C_Str()] = nextIndex;
				indexPtr = level.texMapping.get(tmpPath.C_Str());

				level.textures.add(loadImage(basePath, tmpPath.C_Str()));
			}
			materialTmp.albedoTexIndex = *indexPtr;
		}
		else {
			aiColor3D color(0.0f, 0.0f, 0.0f);
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			materialTmp.albedo = vec4(color.r, color.g, color.b, 1.0f);
		}


		// Roughness (stored in map_Ns, specular highlight component)
		if (mat->GetTextureCount(aiTextureType_SHININESS) > 0) {
			sfz_assert_debug(mat->GetTextureCount(aiTextureType_SHININESS) == 1);

			tmpPath.Clear();
			mat->GetTexture(aiTextureType_SHININESS, 0, &tmpPath);

			const uint32_t* indexPtr = level.texMapping.get(tmpPath.C_Str());
			if (indexPtr == nullptr) {
				PH_LOG(LOG_LEVEL_INFO, "SponzaLoader", "Loaded roughness texture: %s",
					tmpPath.C_Str());

				const uint32_t nextIndex = level.textures.size();
				level.texMapping[tmpPath.C_Str()] = nextIndex;
				indexPtr = level.texMapping.get(tmpPath.C_Str());

				level.textures.add(loadImage(basePath, tmpPath.C_Str()));
			}
			materialTmp.roughnessTexIndex = *indexPtr;
		}
		else {
			aiColor3D color(0.0f, 0.0f, 0.0f);
			mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
			materialTmp.roughness = color.r;
		}

		// Metallic (stored in map_Ka, ambient texture map)
		if (mat->GetTextureCount(aiTextureType_AMBIENT) > 0) {
			sfz_assert_debug(mat->GetTextureCount(aiTextureType_AMBIENT) == 1);

			tmpPath.Clear();
			mat->GetTexture(aiTextureType_AMBIENT, 0, &tmpPath);

			const uint32_t* indexPtr = level.texMapping.get(tmpPath.C_Str());
			if (indexPtr == nullptr) {
				PH_LOG(LOG_LEVEL_INFO, "SponzaLoader", "Loaded metallic texture: %s",
					tmpPath.C_Str());

				const uint32_t nextIndex = level.textures.size();
				level.texMapping[tmpPath.C_Str()] = nextIndex;
				indexPtr = level.texMapping.get(tmpPath.C_Str());

				level.textures.add(loadImage(basePath, tmpPath.C_Str()));
			}
			materialTmp.metallicTexIndex = *indexPtr;
		}
		else {
			aiColor3D color(0.0f, 0.0f, 0.0f);
			mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
			materialTmp.metallic = color.r;
		}

		// Go through all existing materials and try to find one identical to the current one
		uint32_t materialIndex = ~0u;
		for (uint32_t i = 0; i < level.materials.size(); i++) {
			if (approxEqual(level.materials[i], materialTmp)) {
				materialIndex = i;
				break;
			}
		}

		// If material doesn't exist, add it
		if (materialIndex == ~0u) {
			materialIndex = level.materials.size();
			level.materials.add(materialTmp);
		}

		// Set material index to all vertices in mesh and add it to level
		meshTmp.materialIndices.addMany(meshTmp.vertices.size(), uint16_t(materialIndex));

		level.meshes.add(std::move(meshTmp));
	}

	// Process all children
	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		processNode(basePath, level, scene, node->mChildren[i], modelMatrix, normalMatrix);
	}
}

Level loadStaticSceneSponza(
	const char* basePath,
	const char* fileName,
	const mat4& modelMatrix) noexcept
{
	// Create full path
	uint32_t basePathLen = uint32_t(std::strlen(basePath));
	uint32_t fileNameLen = uint32_t(std::strlen(fileName));
	DynString path("", basePathLen + fileNameLen + 2);
	path.printf("%s%s", basePath, fileName);
	if (path.size() < 1) {
		sfz::printErrorMessage("Failed to load model, empty path");
		return Level();
	}

	// Get the real base path from the path
	DynString realBasePath(path.str());
	DynArray<char>& internal = realBasePath.internalDynArray();
	for (uint32_t i = internal.size() - 1; i > 0; i--) {
		const char c = internal[i - 1];
		if (c == '\\' || c == '/') {
			internal[i] = '\0';
			internal.setSize(i + 1);
			break;
		}
	}
	if (realBasePath.size() == path.size()) {
		sfz::printErrorMessage("Failed to find real base path, basePath=\"%s\", fileName=\"%s\"", basePath, fileName);
		return Level();
	}

	// Load model through Assimp
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.str(), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs);
	if (scene == nullptr || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr) {
		sfz::printErrorMessage("Failed to load model \"%s\", error: %s", fileName, importer.GetErrorString());
		return Level();
	}

	Level tmpLevel;

	// Process tree, filling up the list of renderable components along the way
	const mat4 normalMatrix = inverse(transpose(modelMatrix));
	processNode(realBasePath.str(), tmpLevel, scene, scene->mRootNode, modelMatrix, normalMatrix);

	return tmpLevel;
}

} // namespace ph
