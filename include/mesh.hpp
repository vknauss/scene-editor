#pragma once

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "vector_math.hpp"

#include "mesh/attribute.hpp"
#include "mesh/attribute_buffer.hpp"
#include "mesh/attribute_view.hpp"

#if (defined (__clang__) || defined (__GNUC__))
#include <cxxabi.h>
#define CXX_ABI_INCLUDED
#endif


class Mesh {

public:

    friend class MeshVertexBufferWriter;

    using index_t = uint32_t;

    Mesh();

    explicit Mesh(size_t numVertices);

    template<typename T>
    TypedMeshAttributeBuffer<T>& createAttributeBuffer(MeshAttribute attribute);

    template<typename T>
    TypedMeshAttributeBuffer<T>& getAttributeBuffer(MeshAttribute attribute);

    template<typename T>
    const TypedMeshAttributeBuffer<T>& getAttributeBuffer(MeshAttribute attribute) const;

    MeshAttributeBuffer& getAttributeBuffer(MeshAttribute attribute);
    const MeshAttributeBuffer& getAttributeBuffer(MeshAttribute attribute) const;

    MeshAttributeBuffer& getAttributeBuffer(uint32_t index);
    const MeshAttributeBuffer& getAttributeBuffer(uint32_t index) const;

    template<typename ... Types, typename ... AttribArgs>
    MeshAttributeView<TypedMeshAttributeBuffer<Types>...> view(AttribArgs... args);

    template<typename ... Types, typename ... AttribArgs>
    MeshAttributeView<const TypedMeshAttributeBuffer<Types>...> view(AttribArgs... args) const;

    uint32_t numAttributes() const noexcept;

    size_t numVertices() const noexcept;

    void setNumVertices(size_t numVertices);

    std::vector<index_t>& indices() noexcept;

    const std::vector<index_t>& indices() const noexcept;

    bool hasIndices() const noexcept;

private:

    // Internal methods to make implementation of buffer accesses consistent
    
    std::optional<uint32_t> bufferIndex(MeshAttribute) const;

    // Static helper methods allow templating for const / non-const access

    template<typename SELF_T, typename T, template <typename TT> typename RET_T>
    static RET_T<T>* getTypedBuffer(SELF_T* self, uint32_t index);

    template<typename SELF_T, typename T, template <typename TT> typename RET_T>
    static RET_T<T>& getAttributeBuffer(SELF_T* self, MeshAttribute attribute);

    // Member data

    std::vector<std::unique_ptr<MeshAttributeBuffer>> _buffers;
    
    std::map<MeshAttribute, uint32_t> _bufferIndices;

    std::vector<index_t> _indices;

    size_t _numVertices;

};

// Inline implementation

// Constructors

inline Mesh::Mesh() :
        _numVertices(0) {
}

inline Mesh::Mesh(size_t numVertices) :
        _numVertices(numVertices) {
}

// Inline functions

inline uint32_t Mesh::numAttributes() const noexcept {
    return _buffers.size();
}

inline size_t Mesh::numVertices() const noexcept {
    return _numVertices;
}

inline void Mesh::setNumVertices(size_t numVertices) {
    _numVertices = numVertices;
    for (auto& buffer : _buffers) {
        buffer->resize(numVertices);
    }
}

inline std::vector<Mesh::index_t>& Mesh::indices() noexcept {
    return _indices;
}

inline const std::vector<Mesh::index_t>& Mesh::indices() const noexcept {
    return _indices;
}

inline bool Mesh::hasIndices() const noexcept {
    return !_indices.empty();
}

inline std::optional<uint32_t> Mesh::bufferIndex(MeshAttribute attribute) const {
    if (auto it = _bufferIndices.find(attribute); it != _bufferIndices.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Template implementation

template<typename SELF_T, typename T, template <typename TT> typename RET_T>
inline RET_T<T>* Mesh::getTypedBuffer(SELF_T* self, uint32_t index) {
    return dynamic_cast<RET_T<T>*>(self->_buffers[index].get());
}

template<typename SELF_T, typename T, template <typename TT> typename RET_T>
inline RET_T<T>& Mesh::getAttributeBuffer(SELF_T* self, MeshAttribute attribute) {

    // https://stackoverflow.com/questions/1055452/c-get-name-of-type-in-template
    static const auto tName = [] (void) {
        std::string tname = typeid(T).name();
#ifdef CXX_ABI_INCLUDED
        int status;
        auto dname = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        if (status == 0) {
            tname = dname;
            free(dname);
        } else {
#endif
            tname += " (Name may be mangled)";
#ifdef CXX_ABI_INCLUDED
        }
#endif
        return tname;
    };

    if (std::optional<uint32_t> index = self->bufferIndex(attribute)) {
        RET_T<T>* pRet = getTypedBuffer<SELF_T, T, RET_T>(self, index.value());
        if (pRet) return *pRet;
        const auto& info = typeid(T);
        throw std::invalid_argument(std::string("Buffer for mesh attribute: ") + attributeName(attribute) + " does not match type: " + tName());
    }
    throw std::invalid_argument(std::string("Mesh has no buffer for attribute: ") + attributeName(attribute));
}

template<typename T>
inline TypedMeshAttributeBuffer<T>& Mesh::createAttributeBuffer(MeshAttribute attribute) {
    if (std::optional<uint32_t> index = bufferIndex(attribute)) {
        throw std::invalid_argument(std::string("Mesh already has buffer for attribute: ") + attributeName(attribute));
    }
    uint32_t index = _buffers.size();
    _bufferIndices.insert(std::make_pair(attribute, index));
    _buffers.emplace_back(new TypedMeshAttributeBuffer<T>(attribute, _numVertices));
    return *static_cast<TypedMeshAttributeBuffer<T>*>(_buffers.back().get());
}

template<typename T>
inline TypedMeshAttributeBuffer<T>& Mesh::getAttributeBuffer(MeshAttribute attribute) {
    return getAttributeBuffer<Mesh, T, TypedMeshAttributeBuffer>(this, attribute);
}

template<typename T>
inline const TypedMeshAttributeBuffer<T>& Mesh::getAttributeBuffer(MeshAttribute attribute) const {
    return getAttributeBuffer<const Mesh, T, TypedMeshAttributeBuffer>(this, attribute);
}

inline MeshAttributeBuffer& Mesh::getAttributeBuffer(MeshAttribute attribute) {
    if (std::optional<uint32_t> index = bufferIndex(attribute)) {
        return *_buffers[index.value()];
    }
    throw std::invalid_argument(std::string("Mesh has no buffer for attribute: ") + attributeName(attribute));
}

inline const MeshAttributeBuffer& Mesh::getAttributeBuffer(MeshAttribute attribute) const {
    if (std::optional<uint32_t> index = bufferIndex(attribute)) {
        return *_buffers[index.value()];
    }
    throw std::invalid_argument(std::string("Mesh has no buffer for attribute: ") + attributeName(attribute));
}

inline MeshAttributeBuffer& Mesh::getAttributeBuffer(uint32_t index) {
    return *_buffers[index];
}

inline const MeshAttributeBuffer& Mesh::getAttributeBuffer(uint32_t index) const {
    return *_buffers[index];
}

template<typename ... Types, typename ... AttribArgs>
inline MeshAttributeView<TypedMeshAttributeBuffer<Types>...> Mesh::view(AttribArgs... args) {
    static_assert(sizeof...(Types) == sizeof...(AttribArgs), "Number of types and number of arguments must match.");
    return { getAttributeBuffer<Types>(args) ... };
}

template<typename ... Types, typename ... AttribArgs>
inline MeshAttributeView<const TypedMeshAttributeBuffer<Types>...> Mesh::view(AttribArgs... args) const {
    static_assert(sizeof...(Types) == sizeof...(AttribArgs), "Number of types and number of arguments must match.");
    return { getAttributeBuffer<Types>(args)... };
}