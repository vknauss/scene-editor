#include <mesh_vertex_buffer_writer.hpp>

MeshVertexBufferWriter::MeshVertexBufferWriter(const Mesh& mesh) :
        _mesh(mesh) {
}

static bool validateRenderMeshMapping(const Mesh& mesh, const RenderMeshMapping& mapping) noexcept {
    for (const auto& attribMapping : mapping.attributeMappings) {
        try {
            const MeshAttributeBuffer& buffer = mesh.getAttributeBuffer(attribMapping.attribute);
            if (buffer.componentType() != attribMapping.componentType ||
                    buffer.numComponents() != attribMapping.numComponents) {
                return false;
            }
        } catch (std::exception& e) {
            return false;
        }
    }
    return true;
}

static void writeMeshBlock(const Mesh& mesh, MeshRenderer& meshRenderer, const MeshRenderer::Block& block) {
    const RenderMeshMapping& mapping = meshRenderer.getRenderMeshMapping();

    std::vector<const MeshAttributeBuffer*> attribBuffers(mapping.attributeMappings.size());
    for (auto i = 0u; i < attribBuffers.size(); ++i) {
        attribBuffers[i] = &mesh.getAttributeBuffer(mapping.attributeMappings[i].attribute);
    }
    
    ogu::buffer& vb = meshRenderer.getVertexBuffer();
    vb.write(block.vboOffset, block.vboSize, [&] (void* bufferData) {
        unsigned char* ucData = static_cast<unsigned char*>(bufferData);
        for (size_t i = 0u; i < mesh.numVertices(); ++i) {
            for (const auto* buffer : attribBuffers) {
                memcpy(ucData, buffer->elementPtr(i), buffer->elementSize());
                ucData += buffer->elementSize();
            }
        }
    });

    ogu::buffer& ib = meshRenderer.getIndexBuffer();
    ib.write(block.iboOffset, block.iboSize, [&] (void* bufferData) {
        Mesh::index_t* ibData = static_cast<Mesh::index_t*>(bufferData);
        for (Mesh::index_t index : mesh.indices()) {
            *(ibData++) = index + block.vertexOffset;
        }
    });
}

void MeshVertexBufferWriter::write(MeshRenderer& meshRenderer) const {
    // this is commented out since the exceptions thrown by Mesh are more helpful anyway
    // if (!validateRenderMeshMapping(_mesh, mapping)) {
    //     throw std::invalid_argument("Mesh does not contain all the attributes required for the given MeshRenderer.");
    // }

    auto block = meshRenderer.allocateMeshBlock(_mesh.numVertices(), _mesh.indices().size());

    writeMeshBlock(_mesh, meshRenderer, block);
}