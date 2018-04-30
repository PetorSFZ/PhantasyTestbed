#include "GltfLoader.hpp"

#include <sfz/Logging.hpp>
#include <sfz/strings/StackString.hpp>

#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define TINYGLTF_IMPLEMENTATION

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

#include <sfz/Assert.hpp>

namespace ph {

enum class ComponentType : uint32_t {
	INT8 = 5120,
	UINT8 = 5121,
	INT16 = 5122,
	UINT16 = 5123,
	UINT32 = 5125,
	FLOAT32 = 5126,
};

static uint32_t numBytes(ComponentType type)
{
	switch (type) {
	case ComponentType::INT8: return 1;
	case ComponentType::UINT8: return 1;
	case ComponentType::INT16: return 2;
	case ComponentType::UINT16: return 2;
	case ComponentType::UINT32: return 4;
	case ComponentType::FLOAT32: return 4;
	}
	return 0;
}

enum class ComponentDimensions : uint32_t {
	SCALAR = TINYGLTF_TYPE_SCALAR,
	VEC2 = TINYGLTF_TYPE_VEC2,
	VEC3 = TINYGLTF_TYPE_VEC3,
	VEC4 = TINYGLTF_TYPE_VEC4,
	MAT2 = TINYGLTF_TYPE_MAT2,
	MAT3 = TINYGLTF_TYPE_MAT3,
	MAT4 = TINYGLTF_TYPE_MAT4,
};

static uint32_t numDimensions(ComponentDimensions dims)
{
	switch (dims) {
	case ComponentDimensions::SCALAR: return 1;
	case ComponentDimensions::VEC2: return 2;
	case ComponentDimensions::VEC3: return 3;
	case ComponentDimensions::VEC4: return 4;
	case ComponentDimensions::MAT2: return 4;
	case ComponentDimensions::MAT3: return 9;
	case ComponentDimensions::MAT4: return 16;
	}
	return 0;
}

struct DataAccess final {
	const uint8_t* rawPtr = nullptr;
	uint32_t numElements = 0;
	ComponentType compType = ComponentType::UINT8;
	ComponentDimensions compDims = ComponentDimensions::SCALAR;

	template<typename T>
	const T& at(uint32_t index) const noexcept
	{
		return reinterpret_cast<const T*>(rawPtr)[index];
	}
};

static DataAccess accessData(const tinygltf::Model& model, int32_t accessorIdx) noexcept
{
	// Access Accessor
	if (accessorIdx < 0) return DataAccess();
	if (accessorIdx >= int32_t(model.accessors.size())) return DataAccess();
	const tinygltf::Accessor& accessor = model.accessors[accessorIdx];

	// Access BufferView
	if (accessor.bufferView < 0) return DataAccess();
	if (accessor.bufferView >= int32_t(model.bufferViews.size())) return DataAccess();
	const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

	// Access Buffer
	if (bufferView.buffer < 0) return DataAccess();
	if (bufferView.buffer >= int32_t(model.buffers.size())) return DataAccess();
	const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

	// Fill DataAccess struct
	DataAccess tmp;
	tmp.rawPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	tmp.numElements = uint32_t(accessor.count);
	tmp.compType = ComponentType(accessor.componentType);
	tmp.compDims = ComponentDimensions(accessor.type);

	// For now we require that that there is no padding between elements in buffer
	sfz_assert_release(
		bufferView.byteStride == 0 ||
		bufferView.byteStride == size_t(numDimensions(tmp.compDims) * numBytes(tmp.compType)));

	return tmp;
}

static Mesh convertMesh(const tinygltf::Model& model, uint32_t tmpMatIdx) noexcept
{
	Mesh meshOut;

	/*const tinygltf::Scene& scene = model.scenes[model.defaultScene];

	for (int nodeIdx : scene.nodes) {
		const tinygltf::Node& node = model.nodes[nodeIdx];
		int meshIdx = node.mesh;
		const tinygltf::Mesh& mesh = model.meshes[meshIdx];

		for (const tinygltf::Primitive& primitive : mesh.primitives) {

			//const tinygltf::Material& material = model.materials[primitive.material];

			//const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];



		}
	}*/

	// Really stupidly, we are just going to be looking at the first mesh.
	const tinygltf::Mesh& mesh = model.meshes[0];

	// Assume just one primitive (also stupid)
	const tinygltf::Primitive& primitive = mesh.primitives[0];

	// Mode can be:
	// TINYGLTF_MODE_POINTS (0)
	// TINYGLTF_MODE_LINE (1)
 	// TINYGLTF_MODE_LINE_LOOP (2)
	// TINYGLTF_MODE_TRIANGLES (4)
	// TINYGLTF_MODE_TRIANGLE_STRIP (5)
	// TINYGLTF_MODE_TRIANGLE_FAN (6)
	sfz_assert_release(primitive.mode == TINYGLTF_MODE_TRIANGLES);

	sfz_assert_release(primitive.indices >= 0 && primitive.indices < model.accessors.size());

	// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#geometry
	//
	// Allowed attributes:
	// POSITION, NORMAL, TANGENT, TEXCOORD_0, TEXCOORD_1, COLOR_0, JOINTS_0, WEIGHTS_0
	//
	// Stupidly assume position and normal exists
	DataAccess posAccess = accessData(model, primitive.attributes.find("POSITION")->second);
	sfz_assert_release(posAccess.rawPtr != nullptr);
	sfz_assert_release(posAccess.compType == ComponentType::FLOAT32);
	sfz_assert_release(posAccess.compDims == ComponentDimensions::VEC3);

	DataAccess normalAccess = accessData(model, primitive.attributes.find("NORMAL")->second);
	sfz_assert_release(normalAccess.rawPtr != nullptr);
	sfz_assert_release(normalAccess.compType == ComponentType::FLOAT32);
	sfz_assert_release(normalAccess.compDims == ComponentDimensions::VEC3);

	// Create vertices from positions and normals
	// TODO: Texcoords
	sfz_assert_release(posAccess.numElements == normalAccess.numElements);
	meshOut.vertices.create(posAccess.numElements);
	for (uint32_t i = 0; i < posAccess.numElements; i++) {
		Vertex vertex;
		vertex.pos = posAccess.at<vec3>(i);
		vertex.normal = normalAccess.at<vec3>(i);
		vertex.texcoord = vec2(0.0f);
		meshOut.vertices.add(vertex);
	}

	// Create indicess
	DataAccess idxAccess = accessData(model, primitive.indices);
	sfz_assert_release(idxAccess.rawPtr != nullptr);
	sfz_assert_release(idxAccess.compDims == ComponentDimensions::SCALAR);
	if (idxAccess.compType == ComponentType::UINT32) {
		meshOut.indices.create(idxAccess.numElements);
		meshOut.indices.add(&idxAccess.at<uint32_t>(0), idxAccess.numElements);
	}
	else if (idxAccess.compType == ComponentType::UINT16) {
		meshOut.indices.create(idxAccess.numElements);
		for (uint32_t i = 0; i < idxAccess.numElements; i++) {
			meshOut.indices.add(uint32_t(idxAccess.at<uint16_t>(i)));
		}
	}
	else {
		sfz_assert_release(false);
	}

	// Create materialIndices
	// TOOD: Currently only making them up and pointing to 0
	meshOut.materialIndices.create(meshOut.vertices.size());
	meshOut.materialIndices.addMany(meshOut.vertices.size(), tmpMatIdx);

	return meshOut;
}

Mesh loadMeshFromGltf(const char* basePath, const char* gltfPath, uint32_t tmpMatIdx) noexcept
{
	// Append paths
	sfz::StackString256 path;
	path.printf("%s%s", basePath, gltfPath);
	printf("%s\n", path.str);

	// Read model from file
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string error;
	bool result = loader.LoadASCIIFromFile(&model, &error, path.str);

	// Check error string
	if (!error.empty()) {
		SFZ_ERROR("tinygltf", "Error loading \"%s\": %s", path.str, error.c_str());
		return Mesh();
	}

	// Check return code
	if (!result) {
		SFZ_ERROR("tinygltf", "Error loading \"%s\"", path.str);
		return Mesh();
	}

	// Log that model was succesfully loaded
	SFZ_INFO_NOISY("tinygltf", "Model \"%s\" loaded succesfully", path.str);

	// Convert model to PhantasyEngine representation
	return convertMesh(model, tmpMatIdx);
}

} // namespace ph
