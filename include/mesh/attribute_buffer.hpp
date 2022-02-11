#pragma once

#include <cstddef>
#include <vector>


// Virtual base class for mesh attribute buffers

class MeshAttributeBuffer {

public:

    friend class Mesh;

    virtual ~MeshAttributeBuffer() = default;

    const void* data() const;

protected:

    MeshAttributeBuffer() = default;

    void* _data;

private:

    virtual void resize(size_t numElements) = 0;

};

inline const void* MeshAttributeBuffer::data() const {
    return _data;
}

// Templated child class for buffers of various element types

template<typename T>
class TypedMeshAttributeBuffer : public MeshAttributeBuffer {

public:

    friend class Mesh;

    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    // const access to underlying vector
    
    const std::vector<T>& elements() const noexcept;

    // iterator access for range-for and STL algorithm compatibility

    iterator begin() noexcept;

    const_iterator begin() const noexcept;

    iterator end() noexcept;

    const_iterator end() const noexcept;

    const_iterator cbegin() const noexcept;

    const_iterator cend() const noexcept;

private:

    explicit TypedMeshAttributeBuffer(size_t numElements);

    void resize(size_t numElements) override;

    std::vector<T> _elements;

};

// Inline template implementation

// Constructors

template<typename T>
TypedMeshAttributeBuffer<T>::TypedMeshAttributeBuffer(size_t numElements) :
        _elements(numElements) {
    _data = _elements.data();
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
