// glTF/GLB model loader: Luanti's mesh loader for .gltf (JSON) and .glb (binary container) models,
// as reached through IMeshLoader::createMesh(IReadFile *), the entry the scene manager uses for mod models.
#include "CGLTFMeshFileLoader.h"
#include "IAnimatedMesh.h"
#include "IReadFile.h"

#include <cstring>

namespace io {
IReadFile *createMemoryReadFile(const void *memory, long size, const path &fileName,
		bool deleteMemoryWhenDropped);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	// The loader picks the container by file extension; take it from the input's own magic.
	const bool glb = size >= 4 && memcmp(data, "glTF", 4) == 0;
	io::IReadFile *file = io::createMemoryReadFile(
			data, static_cast<long>(size), glb ? "model.glb" : "model.gltf", false);
	scene::CGLTFMeshFileLoader loader;
	scene::IAnimatedMesh *mesh = loader.createMesh(file);
	file->drop();
	if (!mesh)
		return -1;
	mesh->drop();
	return 0;
}
