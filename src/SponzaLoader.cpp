// See 'LICENSE_PHANTASY_ENGINE' for copyright and contributors.

#include "SponzaLoader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <sfz/strings/StringHashers.hpp>
#include <sfz/Logging.hpp>
#include <sfz/containers/HashMap.hpp>
#include <sfz/math/MathSupport.hpp>
#include <sfz/strings/DynString.hpp>
#include <sfz/strings/StackString.hpp>
#include <sfz/containers/HashMap.hpp>


namespace ph {

using namespace sfz;

static bool operator== (const Material& lhs, const Material& rhs) noexcept
{
	return
		lhs.albedo == rhs.albedo &&
		lhs.emissive == rhs.emissive &&
		lhs.roughness == rhs.roughness &&
		lhs.metallic == rhs.metallic &&

		lhs.albedoTexIndex == rhs.albedoTexIndex &&
		lhs.metallicRoughnessTexIndex == rhs.metallicRoughnessTexIndex &&
		lhs.normalTexIndex == rhs.normalTexIndex &&
		lhs.occlusionTexIndex == rhs.occlusionTexIndex &&
		lhs.emissiveTexIndex == rhs.emissiveTexIndex;
}

static uint8_t f32ToU8(float f) noexcept
{
	return uint8_t(std::roundf(f * 255.0f));
}
static vec4_u8 toSFZ(const aiColor3D& c) noexcept
{
	vec4_u8 tmp;
	tmp.x = f32ToU8(c.r);
	tmp.y = f32ToU8(c.g);
	tmp.z = f32ToU8(c.b);
	tmp.w = f32ToU8(1.0f);
	return tmp;
}

static vec3 toSFZ(const aiVector3D& v)
{
	return vec3(v.x, v.y, v.z);
}

static void processNode(
	const char* basePath,
	LevelAssets& level,
	HashMap<StackString256, uint32_t>& texMapping,
	const aiScene* scene,
	aiNode* node,
	const mat4& modelMatrix,
	const mat4& normalMatrix) noexcept
{
	aiString tmpPath;
	aiString tmpPath2;

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

			const uint32_t* indexPtr = texMapping.get(tmpPath.C_Str());
			if (indexPtr == nullptr) {
				const uint32_t nextIndex = level.textures.size();
				texMapping[tmpPath.C_Str()] = nextIndex;
				indexPtr = texMapping.get(tmpPath.C_Str());

				level.textures.add(loadImage(basePath, tmpPath.C_Str()));
			}
			materialTmp.albedoTexIndex = *indexPtr;
		}
		else {
			aiColor3D color(0.0f, 0.0f, 0.0f);
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			materialTmp.albedo = toSFZ(color);
		}

		// Roughness and metallic
		// Roughness stored in map_Ns, specular highlight component
		// Metallic stored in map_Ka, ambient texture map
		if (mat->GetTextureCount(aiTextureType_SHININESS) > 0 &&
			mat->GetTextureCount(aiTextureType_AMBIENT) > 0) {

			sfz_assert_debug(mat->GetTextureCount(aiTextureType_SHININESS) == 1);
			sfz_assert_debug(mat->GetTextureCount(aiTextureType_AMBIENT) == 1);

			tmpPath.Clear();
			mat->GetTexture(aiTextureType_SHININESS, 0, &tmpPath);

			tmpPath2.Clear();
			mat->GetTexture(aiTextureType_AMBIENT, 0, &tmpPath2);

			str512 combinedStr("%s%s", tmpPath.C_Str(), tmpPath2.C_Str());

			const uint32_t* indexPtr = texMapping.get(combinedStr);
			if (indexPtr == nullptr) {

				const uint32_t nextIndex = level.textures.size();
				texMapping[combinedStr] = nextIndex;
				indexPtr = texMapping.get(combinedStr);

				Image roughnessImage = loadImage(basePath, tmpPath.C_Str());
				Image metallicImage = loadImage(basePath, tmpPath2.C_Str());

				sfz_assert_debug(roughnessImage.rawData.data() != nullptr);
				sfz_assert_debug(metallicImage.rawData.data() != nullptr);
				sfz_assert_debug(roughnessImage.width == metallicImage.width);
				sfz_assert_debug(roughnessImage.height == metallicImage.height);
				sfz_assert_debug(roughnessImage.bytesPerPixel == 1);
				sfz_assert_debug(metallicImage.bytesPerPixel == 1);

				Image combined;
				combined.rawData.setCapacity(
					uint32_t(roughnessImage.width * roughnessImage.height * 2));
				combined.rawData.setSize(combined.rawData.capacity());
				combined.width = roughnessImage.width;
				combined.height = roughnessImage.height;
				combined.bytesPerPixel = 2;
				
				for (uint32_t i = 0; i < roughnessImage.rawData.size(); i++) {
					uint8_t roughness = roughnessImage.rawData[i];
					uint8_t metallic = metallicImage.rawData[i];
					combined.rawData[i*2] = metallic;
					combined.rawData[i*2 + 1] = roughness;
				}

				level.textures.add(std::move(combined));
			}
			materialTmp.metallicRoughnessTexIndex = uint16_t(*indexPtr);
		}
		else {
			aiColor3D color(0.0f, 0.0f, 0.0f);
			mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
			materialTmp.roughness = f32ToU8(color.r);

			color = aiColor3D (0.0f, 0.0f, 0.0f);
			mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
			materialTmp.metallic = f32ToU8(color.r);
		}

		// Go through all existing materials and try to find one identical to the current one
		uint32_t materialIndex = ~0u;
		for (uint32_t i = 0; i < level.materials.size(); i++) {
			if (level.materials[i] == materialTmp) {
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
		processNode(basePath, level, texMapping, scene, node->mChildren[i], modelMatrix, normalMatrix);
	}
}

bool loadStaticSceneSponza(
	const char* basePath,
	const char* fileName,
	LevelAssets& assets,
	const mat4& modelMatrix) noexcept
{
	// Create full path
	uint32_t basePathLen = uint32_t(std::strlen(basePath));
	uint32_t fileNameLen = uint32_t(std::strlen(fileName));
	DynString path("", basePathLen + fileNameLen + 2);
	path.printf("%s%s", basePath, fileName);
	if (path.size() < 1) {
		SFZ_ERROR("SponzaLoader", "Failed to load model, empty path");
		return false;
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
		SFZ_ERROR("SponzaLoader", "Failed to find real base path, basePath=\"%s\", fileName=\"%s\"", basePath, fileName);
		return false;
	}

	// Load model through Assimp
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.str(), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs);
	if (scene == nullptr || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr) {
		SFZ_ERROR("SponzaLoader", "Failed to load model \"%s\", error: %s", fileName, importer.GetErrorString());
		return false;
	}

	// Process tree, filling up the list of renderable components along the way
	const mat4 normalMatrix = inverse(transpose(modelMatrix));
	HashMap<StackString256, uint32_t> texMapping;
	processNode(realBasePath.str(), assets, texMapping, scene, scene->mRootNode, modelMatrix, normalMatrix);

	return true;
}

} // namespace ph
