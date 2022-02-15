#pragma once

#include <cstdint>

#include <vector>

#include <ogu/vertex_array.h>

#include "mesh.hpp"


struct RenderMeshMapping {

    struct AttributeMapping {
        MeshAttribute attribute;
        MeshAttributeComponentType componentType;
        int numComponents;
    };

    std::vector<AttributeMapping> attributeMappings;

};


struct VertexBufferLayout {
    uint32_t vertexSize;
    std::vector<ogu::vertex_attrib_description> attributes;
};

class MeshRenderer {

public:

    struct Block {
        uintptr_t vboOffset, iboOffset;      // the offset in bytes into the vbo/ibo buffers
        size_t vboSize, iboSize;             // the size in bytes of the data in the vbo/ibo buffers
        uint32_t vertexOffset, indexOffset;  // the offset in elements of the first vertex/index represented by this block
    };

    MeshRenderer(const RenderMeshMapping& mapping, size_t vboSize, size_t iboSize);

    const RenderMeshMapping& getRenderMeshMapping() const;

    // const VertexBufferLayout& getVertexBufferLayout() const;

    Block allocateMeshBlock(size_t numVertices, size_t numIndices);

    ogu::buffer& getVertexBuffer();
    ogu::buffer& getIndexBuffer();

    const ogu::buffer& getVertexBuffer() const;
    const ogu::buffer& getIndexBuffer() const;
    const ogu::vertex_array& getVertexArray() const;

private:

    RenderMeshMapping _renderMeshMapping;

    // VertexBufferLayout _vertexBufferLayout;

    ogu::buffer _vbo;
    ogu::buffer _ibo;

    ogu::vertex_array _vao;

    std::vector<Block> _freeBlocks;

    size_t _vertexSize, _indexSize;

};

inline const RenderMeshMapping& MeshRenderer::getRenderMeshMapping() const {
    return _renderMeshMapping;
}

// inline const VertexBufferLayout& MeshRenderer::getVertexBufferLayout() const {
//     return _vertexBufferLayout;
// }

inline ogu::buffer& MeshRenderer::getVertexBuffer() {
    return _vbo;
}

inline ogu::buffer& MeshRenderer::getIndexBuffer() {
    return _ibo;
}

inline const ogu::buffer& MeshRenderer::getVertexBuffer() const {
    return _vbo;
}

inline const ogu::buffer& MeshRenderer::getIndexBuffer() const {
    return _ibo;
}

inline const ogu::vertex_array& MeshRenderer::getVertexArray() const {
    return _vao;
}