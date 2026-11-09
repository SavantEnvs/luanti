// Known-answer checks for the glTF loader, built with the project's normal flags. Loads the devtest
// models and prints one "PASS <name>" / "FAIL <name>" line per check; mayhem/test.sh counts them.
// The expected values are the ones src/test/client/test_irr_gltf_mesh_loader.cpp asserts.
#include "CGLTFMeshFileLoader.h"
#include "IAnimatedMesh.h"
#include "IMeshBuffer.h"
#include "IReadFile.h"
#include "S3DVertex.h"
#include "SkinnedMesh.h"
#include "SSkinMeshBuffer.h"
#include "irr_v2d.h"
#include "irr_v3d.h"
#include "matrix4.h"

#include <cstdio>
#include <string>

namespace io {
IReadFile *createMemoryReadFile(const void *memory, long size, const path &fileName,
		bool deleteMemoryWhenDropped);
}

static void check(const char *name, bool ok)
{
	printf("%s %s\n", ok ? "PASS" : "FAIL", name);
}

static scene::IAnimatedMesh *load(const std::string &dir, const char *file)
{
	std::string path = dir + "/" + file;
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return nullptr;
	std::string data;
	char buf[65536];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		data.append(buf, n);
	fclose(f);
	io::IReadFile *rf = io::createMemoryReadFile(data.data(), data.size(), file, false);
	scene::CGLTFMeshFileLoader loader;
	scene::IAnimatedMesh *mesh = loader.createMesh(rf);
	rf->drop();
	return mesh;
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 2;
	const std::string dir = argv[1];
	using video::S3DVertex;

	for (const char *name : {"gltf_minimal_triangle.gltf", "gltf_triangle_with_vertex_stride.gltf",
			"gltf_triangle_without_indices.gltf"}) {
		auto *mesh = load(dir, name);
		bool ok = mesh && mesh->getMeshBufferCount() == 1
				&& mesh->getMeshBuffer(0)->getVertexCount() == 3
				&& mesh->getMeshBuffer(0)->getIndexCount() == 3;
		if (ok) {
			auto *v = static_cast<const S3DVertex *>(mesh->getMeshBuffer(0)->getVertices());
			auto *i = static_cast<const u16 *>(mesh->getMeshBuffer(0)->getIndices());
			ok = v[0].Pos == v3f{0, 0, 0} && v[1].Pos == v3f{1, 0, 0} && v[2].Pos == v3f{0, 1, 0}
					&& i[0] == 2 && i[1] == 1 && i[2] == 0;
		}
		check(name, ok);
		if (mesh)
			mesh->drop();
	}

	for (const char *name : {"gltf_blender_cube.gltf", "gltf_blender_cube.glb"}) {
		auto *mesh = load(dir, name);
		bool ok = mesh && mesh->getMeshBufferCount() == 1;
		if (ok) {
			auto *mb = dynamic_cast<scene::SSkinMeshBuffer *>(mesh->getMeshBuffer(0));
			ok = mb && mb->getVertexCount() == 24 && mb->getIndexCount() == 36;
			if (ok) {
				core::matrix4 scale;
				scale.setScale(v3f(10.0f));
				auto *v = static_cast<const S3DVertex *>(mb->getVertices());
				auto *i = static_cast<const u16 *>(mb->getIndices());
				ok = mb->Transformation == scale
						&& v[0].Pos == v3f{-1, -1, -1} && v[21].Pos == v3f{1, 1, 1}
						&& v[0].Normal == v3f{-1, 0, 0} && v[23].Normal == v3f{1, 0, 0}
						&& v[0].TCoords == v2f{0.375f, 1.0f}
						&& i[0] == 16 && i[1] == 5 && i[2] == 22 && i[35] == 0;
			}
		}
		check(name, ok);
		if (mesh)
			mesh->drop();
	}

	{
		auto *mesh = load(dir, "gltf_snow_man.gltf");
		bool ok = mesh && mesh->getMeshBufferCount() == 3;
		if (ok) {
			auto *v0 = static_cast<const S3DVertex *>(mesh->getMeshBuffer(0)->getVertices());
			auto *v1 = static_cast<const S3DVertex *>(mesh->getMeshBuffer(1)->getVertices());
			auto *v2 = static_cast<const S3DVertex *>(mesh->getMeshBuffer(2)->getVertices());
			ok = v0[0].Pos == v3f{3, 24, -3} && v1[2].Pos == v3f{5, 10, 5}
					&& v2[1].Pos == v3f{4, 10, -4};
		}
		check("gltf_snow_man.gltf", ok);
		if (mesh)
			mesh->drop();
	}

	for (const char *name : {"gltf_simple_skin.gltf", "gltf_simple_skin_step.gltf"}) {
		auto *mesh = load(dir, name);
		auto *skinned = dynamic_cast<scene::SkinnedMesh *>(mesh);
		bool ok = skinned && skinned->getJointCount() == 3;
		if (ok) {
			const auto &anim = skinned->getAnimation(0);
			ok = anim.joint_keys.size() == 1 && anim.joint_keys[0].keys.rotation.frames.size() == 12
					&& anim.joint_keys[0].keys.rotation.frames.back().time == 5.5f
					&& anim.joint_keys[0].keys.rotation.interpolate == (std::string(name) == "gltf_simple_skin.gltf");
		}
		check(name, ok);
		if (mesh)
			mesh->drop();
	}

	{
		auto *mesh = load(dir, "gltf_simple_sparse_accessor.gltf");
		check("gltf_simple_sparse_accessor.gltf", mesh && mesh->getMeshBufferCount() == 1);
		if (mesh)
			mesh->drop();
	}

	for (const char *name : {"empty.gltf", "invalid_bufferview_bounds.gltf", "json_missing_brace.gltf"}) {
		auto *mesh = load(dir + "/../invalid", name);
		check(name, mesh == nullptr);
		if (mesh)
			mesh->drop();
	}
	return 0;
}
