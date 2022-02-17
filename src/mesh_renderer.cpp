#include <mesh_renderer.hpp>

static constexpr GLenum componentTypeGLEnum(MeshAttributeComponentType type) {
    switch (type) {
    case MeshAttributeComponentType::FLOAT:
        return GL_FLOAT;
    case MeshAttributeComponentType::INT:
        return GL_INT;
    case MeshAttributeComponentType::UINT:
        return GL_UNSIGNED_INT;
    }
    return 0;
}

static constexpr bool componentTypeIsInteger(MeshAttributeComponentType type) {
    switch (type) {
    case MeshAttributeComponentType::INT:
    case MeshAttributeComponentType::UINT:
        return true;
    default:
        return false;
    }
}

static ogu::vertex_buffer_binding createVertexBufferBinding(const ogu::buffer& vbo, const RenderMeshMapping& mapping) {
    std::vector<ogu::vertex_attrib_description> attribDescriptions;
    attribDescriptions.reserve(mapping.attributeMappings.size());
    size_t stride = 0;
    for (uint32_t i = 0u; i < mapping.attributeMappings.size(); ++i) {
        const auto& attribMapping = mapping.attributeMappings[i];
        attribDescriptions.push_back(
            ogu::vertex_attrib_description(i, attribMapping.numComponents,
            componentTypeGLEnum(attribMapping.componentType), stride,
            componentTypeIsInteger(attribMapping.componentType), false));
        stride += componentSize(attribMapping.componentType) * attribMapping.numComponents;
    }
    return ogu::vertex_buffer_binding(vbo, attribDescriptions, stride, false);
}

MeshRenderer::MeshRenderer(const RenderMeshMapping& mapping, size_t vboSize, size_t iboSize) :
        _renderMeshMapping(mapping),
        _vbo(vboSize),
        _ibo(iboSize),
        _vao({createVertexBufferBinding(_vbo, mapping)}) {
    _freeBlocks.push_back(Block {
        .vboOffset = 0,
        .iboOffset = 0,
        .vboSize = vboSize,
        .iboSize = iboSize,
        .vertexOffset = 0,
        .indexOffset = 0
    });
    _vertexSize = 0;
    for (const auto& attrib : _renderMeshMapping.attributeMappings) {
        _vertexSize += componentSize(attrib.componentType) * attrib.numComponents;
    }
    _indexSize = sizeof(Mesh::index_t);
    
    if (iboSize > 0) {
        _vao.bind();
        _ibo.bind(GL_ELEMENT_ARRAY_BUFFER);
    }
}

MeshRenderer::Block MeshRenderer::allocateMeshBlock(size_t numVertices, size_t numIndices) {
    size_t vboSize = numVertices * _vertexSize;
    size_t iboSize = numIndices * _indexSize;
    for (auto& freeBlock : _freeBlocks) {
        if (freeBlock.vboSize >= vboSize && freeBlock.iboSize >= iboSize) {
            Block block = freeBlock;

            freeBlock.vboSize -= vboSize;
            freeBlock.iboSize -= iboSize;
            freeBlock.vboOffset += vboSize;
            freeBlock.vboOffset += iboSize;
            freeBlock.vertexOffset += numVertices;
            freeBlock.indexOffset += numIndices;

            block.vboSize = vboSize;
            block.iboSize = iboSize;

            return block;
        }
    }
    throw std::runtime_error("No space in MeshRenderer.");
}