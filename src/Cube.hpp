#pragma once

#include <sfz/math/Vector.hpp>
#include <sfz/memory/Allocator.hpp>

#include <ph/rendering/Mesh.hpp>

using sfz::Allocator;
using sfz::vec2;
using sfz::vec3;

// Cube model
// ------------------------------------------------------------------------------------------------

const vec3 CUBE_POSITIONS[] = {
	// x, y, z
	// Left
	vec3(-0.5f, -0.5f, -0.5f), // 0, left-bottom-back
	vec3(-0.5f, -0.5f, 0.5f), // 1, left-bottom-front
	vec3(-0.5f, 0.5f, -0.5f), // 2, left-top-back
	vec3(-0.5f, 0.5f, 0.5f), // 3, left-top-front

	// Right
	vec3(0.5f, -0.5f, -0.5f), // 4, right-bottom-back
	vec3(0.5f, -0.5f, 0.5f), // 5, right-bottom-front
	vec3(0.5f, 0.5f, -0.5f), // 6, right-top-back
	vec3(0.5f, 0.5f, 0.5f), // 7, right-top-front

	// Bottom
	vec3(-0.5f, -0.5f, -0.5f), // 8, left-bottom-back
	vec3(-0.5f, -0.5f, 0.5f), // 9, left-bottom-front
	vec3(0.5f, -0.5f, -0.5f), // 10, right-bottom-back
	vec3(0.5f, -0.5f, 0.5f), // 11, right-bottom-front

	// Top
	vec3(-0.5f, 0.5f, -0.5f), // 12, left-top-back
	vec3(-0.5f, 0.5f, 0.5f), // 13, left-top-front
	vec3(0.5f, 0.5f, -0.5f), // 14, right-top-back
	vec3(0.5f, 0.5f, 0.5f), // 15, right-top-front

	// Back
	vec3(-0.5f, -0.5f, -0.5f), // 16, left-bottom-back
	vec3(-0.5f, 0.5f, -0.5f), // 17, left-top-back
	vec3(0.5f, -0.5f, -0.5f), // 18, right-bottom-back
	vec3(0.5f, 0.5f, -0.5f), // 19, right-top-back

	// Front
	vec3(-0.5f, -0.5f, 0.5f), // 20, left-bottom-front
	vec3(-0.5f, 0.5f, 0.5f), // 21, left-top-front
	vec3(0.5f, -0.5f, 0.5f), // 22, right-bottom-front
	vec3(0.5f, 0.5f, 0.5f)  // 23, right-top-front
};

const vec3 CUBE_NORMALS[] = {
	// x, y, z
	// Left
	vec3(-1.0f, 0.0f, 0.0f), // 0, left-bottom-back
	vec3(-1.0f, 0.0f, 0.0f), // 1, left-bottom-front
	vec3(-1.0f, 0.0f, 0.0f), // 2, left-top-back
	vec3(-1.0f, 0.0f, 0.0f), // 3, left-top-front

	// Right
	vec3(1.0f, 0.0f, 0.0f), // 4, right-bottom-back
	vec3(1.0f, 0.0f, 0.0f), // 5, right-bottom-front
	vec3(1.0f, 0.0f, 0.0f), // 6, right-top-back
	vec3(1.0f, 0.0f, 0.0f), // 7, right-top-front

	// Bottom
	vec3(0.0f, -1.0f, 0.0f), // 8, left-bottom-back
	vec3(0.0f, -1.0f, 0.0f), // 9, left-bottom-front
	vec3(0.0f, -1.0f, 0.0f), // 10, right-bottom-back
	vec3(0.0f, -1.0f, 0.0f), // 11, right-bottom-front

	// Top
	vec3(0.0f, 1.0f, 0.0f), // 12, left-top-back
	vec3(0.0f, 1.0f, 0.0f), // 13, left-top-front
	vec3(0.0f, 1.0f, 0.0f), // 14, right-top-back
	vec3(0.0f, 1.0f, 0.0f), // 15, right-top-front

	// Back
	vec3(0.0f, 0.0f, -1.0f), // 16, left-bottom-back
	vec3(0.0f, 0.0f, -1.0f), // 17, left-top-back
	vec3(0.0f, 0.0f, -1.0f), // 18, right-bottom-back
	vec3(0.0f, 0.0f, -1.0f), // 19, right-top-back

	// Front
	vec3(0.0f, 0.0f, 1.0f), // 20, left-bottom-front
	vec3(0.0f, 0.0f, 1.0f), // 21, left-top-front
	vec3(0.0f, 0.0f, 1.0f), // 22, right-bottom-front
	vec3(0.0f, 0.0f, 1.0f)  // 23, right-top-front
};

const vec2 CUBE_TEXCOORDS[] = {
	// u, v
	// Left
	vec2(0.0f, 0.0f), // 0, left-bottom-back
	vec2(1.0f, 0.0f), // 1, left-bottom-front
	vec2(0.0f, 1.0f), // 2, left-top-back
	vec2(1.0f, 1.0f), // 3, left-top-front

	// Right
	vec2(1.0f, 0.0f), // 4, right-bottom-back
	vec2(0.0f, 0.0f), // 5, right-bottom-front
	vec2(1.0f, 1.0f), // 6, right-top-back
	vec2(0.0f, 1.0f), // 7, right-top-front

	// Bottom
	vec2(0.0f, 0.0f), // 8, left-bottom-back
	vec2(0.0f, 1.0f), // 9, left-bottom-front
	vec2(1.0f, 0.0f), // 10, right-bottom-back
	vec2(1.0f, 1.0f), // 11, right-bottom-front

	// Top
	vec2(0.0f, 1.0f), // 12, left-top-back
	vec2(0.0f, 0.0f), // 13, left-top-front
	vec2(1.0f, 1.0f), // 14, right-top-back
	vec2(1.0f, 0.0f), // 15, right-top-front

	// Back
	vec2(1.0f, 0.0f), // 16, left-bottom-back
	vec2(1.0f, 1.0f), // 17, left-top-back
	vec2(0.0f, 0.0f), // 18, right-bottom-back
	vec2(0.0f, 1.0f), // 19, right-top-back

	// Front
	vec2(0.0f, 0.0f), // 20, left-bottom-front
	vec2(0.0f, 1.0f), // 21, left-top-front
	vec2(1.0f, 0.0f), // 22, right-bottom-front
	vec2(1.0f, 1.0f)  // 23, right-top-front
};

constexpr uint32_t CUBE_INDICES[] = {
	// Left
	0, 1, 2,
	3, 2, 1,

	// Right
	5, 4, 7,
	6, 7, 4,

	// Bottom
	8, 10, 9,
	11, 9, 10,

	// Top
	13, 15, 12,
	14, 12, 15,

	// Back
	18, 16, 19,
	17, 19, 16,

	// Front
	20, 22, 21,
	23, 21, 22
};

constexpr uint32_t CUBE_NUM_VERTICES = sizeof(CUBE_POSITIONS) / sizeof(vec3);
constexpr uint32_t CUBE_NUM_INDICES = sizeof(CUBE_INDICES) / sizeof(uint32_t);

inline ph::Mesh createCubeMesh(Allocator* allocator) noexcept
{
	// Create mesh from hardcoded values
	ph::Mesh mesh;

	// Vertices
	mesh.vertices.create(CUBE_NUM_VERTICES, allocator);
	mesh.vertices.addMany(CUBE_NUM_VERTICES);
	for (uint32_t i = 0; i < CUBE_NUM_VERTICES; i++) {
		mesh.vertices[i].pos = CUBE_POSITIONS[i];
		mesh.vertices[i].normal = CUBE_NORMALS[i];
		mesh.vertices[i].texcoord = CUBE_TEXCOORDS[i];
	}

	// Components
	ph::MeshComponent comp;
	comp.materialIdx = 0;
	comp.indices.create(CUBE_NUM_INDICES, allocator);
	comp.indices.add(CUBE_INDICES, CUBE_NUM_INDICES);
	mesh.components.create(1, allocator);
	mesh.components.add(std::move(comp));

	// Material
	ph::MaterialUnbound material;
	material.albedo = sfz::vec4_u8(255, 0, 0, 255);
	material.emissive = sfz::vec3_u8(255, 0, 0);
	material.roughness = 255;
	material.metallic = 0;
	mesh.materials.create(1, allocator);
	mesh.materials.add(material);

	return mesh;
}
