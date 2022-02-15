#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <vector>

#include "vector_math.hpp"
#include "mesh/attribute.hpp"
#include "mesh/component_type_helper.hpp"


// Virtual base class for mesh attribute buffers

class MeshAttributeBuffer {

public:

    friend class Mesh;

    virtual ~MeshAttributeBuffer() = default;

    const void* data() const;

    void* data();

    const void* elementPtr(size_t i) const;

    void* elementPtr(size_t i);

    virtual MeshAttributeComponentType componentType() const noexcept = 0;

    virtual int numComponents() const noexcept = 0;

    size_t elementSize() const noexcept;

    MeshAttribute getAttribute() const noexcept;

protected:

    explicit MeshAttributeBuffer(MeshAttribute attrib);

    void* _data;

    MeshAttribute _attrib;

private:

    virtual void resize(size_t numElements) = 0;

};

inline MeshAttributeBuffer::MeshAttributeBuffer(MeshAttribute attrib) :
        _attrib(attrib) {
}

inline const void* MeshAttributeBuffer::data() const {
    return _data;
}

inline void* MeshAttributeBuffer::data() {
    return _data;
}

inline const void* MeshAttributeBuffer::elementPtr(size_t i) const {
    return (unsigned char*) _data + i * elementSize();
}

inline void* MeshAttributeBuffer::elementPtr(size_t i) {
    return (unsigned char*) _data + i * elementSize();
}

inline constexpr size_t componentSize(MeshAttributeComponentType type) {
    switch (type) {
    case MeshAttributeComponentType::FLOAT:
        return sizeof(float);  // note: this is not 100% guaranteed to be the same size as an OpenGL float... not sure how to make this truly portable
    case MeshAttributeComponentType::INT:
        return sizeof(int32_t);
    case MeshAttributeComponentType::UINT:
        return sizeof(uint32_t);
    }
    return 0;
}

inline size_t MeshAttributeBuffer::elementSize() const noexcept {
    return componentSize(componentType()) * numComponents();
}

inline MeshAttribute MeshAttributeBuffer::getAttribute() const noexcept {
    return _attrib;
}

// Templated child class for buffers of various element types

template<typename T>
class TypedMeshAttributeBuffer : public MeshAttributeBuffer {

public:

    friend class Mesh;

    using value_type = T;

    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    // used to know how to interpret underlying data in vertex buffers

    MeshAttributeComponentType componentType() const noexcept override;

    int numComponents() const noexcept override;

    // const access to underlying vector
    
    const std::vector<T>& elements() const noexcept;

    // iterator access for range-for and STL algorithm compatibility

    iterator begin() noexcept;

    const_iterator begin() const noexcept;

    iterator end() noexcept;

    const_iterator end() const noexcept;

    const_iterator cbegin() const noexcept;

    const_iterator cend() const noexcept;

    T& operator[](size_t i) noexcept;

    const T& operator[](size_t) const noexcept;

    // assignment

    // assign the entire buffer to val, or a value_type constructed from val
    template<typename ... Assign>
    std::enable_if_t<std::is_constructible_v<value_type, Assign...>>
    assign(Assign... val) noexcept;

    // use an initializer list of value_type to fill the buffer
    void assign(const std::initializer_list<value_type>& il);

    // use an initializer list of compatible types to fill the buffer
    // note that this is a more specific specialization than the above by-value assignment template,
    // so if you mean to assign the entire buffer to a value using an initializer list,
    // either cast the initializer list to value_type or just pass in the list items directly,
    // since the above template is variadic
    template<typename Assign>
    std::enable_if_t<std::is_constructible_v<value_type, Assign>>
    assign(const std::initializer_list<Assign>& il);

private:

    explicit TypedMeshAttributeBuffer(MeshAttribute attrib, size_t numElements);

    void resize(size_t numElements) override;

    std::vector<T> _elements;

};

// Inline template implementation

// Constructors

template<typename T>
TypedMeshAttributeBuffer<T>::TypedMeshAttributeBuffer(MeshAttribute attrib, size_t numElements) :
        MeshAttributeBuffer(attrib),
        _elements(numElements) {
    _data = _elements.data();
}

// Overriden functions

template<typename T>
inline MeshAttributeComponentType TypedMeshAttributeBuffer<T>::componentType() const noexcept {
    return ComponentTypeHelper<T>::componentType;
}

template<typename T>
inline int TypedMeshAttributeBuffer<T>::numComponents() const noexcept {
    return ComponentTypeHelper<T>::numComponents;
}

// Iterator access

template<typename T>
inline typename TypedMeshAttributeBuffer<T>::iterator TypedMeshAttributeBuffer<T>::begin() noexcept {
    return _elements.begin();
}

template<typename T>
inline typename TypedMeshAttributeBuffer<T>::const_iterator TypedMeshAttributeBuffer<T>::begin() const noexcept {
    return _elements.begin();
}

template<typename T>
inline typename TypedMeshAttributeBuffer<T>::iterator TypedMeshAttributeBuffer<T>::end() noexcept {
    return _elements.end();
}

template<typename T>
inline typename TypedMeshAttributeBuffer<T>::const_iterator TypedMeshAttributeBuffer<T>::end() const noexcept {
    return _elements.end();
}

template<typename T>
inline typename TypedMeshAttributeBuffer<T>::const_iterator TypedMeshAttributeBuffer<T>::cbegin() const noexcept {
    return _elements.cbegin();
}

template<typename T>
inline typename TypedMeshAttributeBuffer<T>::const_iterator TypedMeshAttributeBuffer<T>::cend() const noexcept {
    return _elements.cend();
}

// operator[]

template<typename T>
inline T& TypedMeshAttributeBuffer<T>::operator[](size_t i) noexcept {
    return _elements[i];
}


template<typename T>
inline const T& TypedMeshAttributeBuffer<T>::operator[](size_t i) const noexcept {
    return _elements[i];
}

// Other methods

template<typename T>
inline const std::vector<T>& TypedMeshAttributeBuffer<T>::elements() const noexcept {
    return _elements;
}

template<typename T>
inline void TypedMeshAttributeBuffer<T>::resize(size_t numElements) {
    _elements.resize(numElements);
    _data = _elements.data();
}

template<typename T>
template<typename ... Assign>
inline std::enable_if_t<std::is_constructible_v<T, Assign...>>
TypedMeshAttributeBuffer<T>::assign(Assign... val) noexcept {
    _elements.assign(_elements.size(), value_type(val...));
}

template<typename T>
void TypedMeshAttributeBuffer<T>::assign(const std::initializer_list<T>& il) {
    if (il.size() != _elements.size()) throw std::invalid_argument("Initializer list size not equal to buffer size.");
    _elements.assign(il);
}

template<typename T>
template<typename Assign>
std::enable_if_t<std::is_constructible_v<T, Assign>>
TypedMeshAttributeBuffer<T>::assign(const std::initializer_list<Assign>& il) {
    if (il.size() != _elements.size()) throw std::invalid_argument("Initializer list size not equal to buffer size.");
    // _elements.assign(il);
    std::transform(il.begin(), il.end(), _elements.begin(), [] (const auto& av) { return value_type(av); });
}
