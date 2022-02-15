#pragma once

#include "mesh.hpp"
#include "mesh_renderer.hpp"


class MeshVertexBufferWriter {

public:

    explicit MeshVertexBufferWriter(const Mesh& mesh);

    void write(MeshRenderer& meshRenderer) const;

private:

    const Mesh& _mesh;

};