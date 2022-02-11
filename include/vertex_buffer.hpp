#pragma once

#include <cstdint>
#include <vector>

#include <ogu/buffer.h>
#include <ogu/vertex_array.h>


struct VertexBufferLayout {
    uint32_t vertexSize;
    std::vector<ogu::vertex_attrib_description> attributes;
};

class VertexBuffer {

public:

    VertexBuffer(const VertexBufferLayout& layout, size_t size);

    const ogu::buffer& getBuffer() const;

    const VertexBufferLayout& getLayout() const;

private:

    struct FreeListEntry {
        uintptr_t offset;
        size_t size;
    };

    ogu::buffer _buffer;

    std::vector<FreeListEntry> _freeBlocks;

    VertexBufferLayout _layout;

};

inline const ogu::buffer& VertexBuffer::getBuffer() const {
    return _buffer;
}

inline const VertexBufferLayout& VertexBuffer::getLayout() const {
    return _layout;
}