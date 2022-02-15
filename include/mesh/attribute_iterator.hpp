#pragma once

#include <iostream>
#include <tuple>

#include <vvm/string.hpp>

#include "attribute_buffer.hpp"


template<typename ... Types>
class MeshAttributeView {

public:

    class iterator;

    MeshAttributeView(TypedMeshAttributeBuffer<Types>& ... buffers);

    iterator begin();

    iterator end();

private:

    std::tuple<TypedMeshAttributeBuffer<Types>& ...> _buffers;

};


// https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
template<typename ... Types>
class MeshAttributeView<Types...>::iterator {
public:
    class pointer_t;

    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<Types& ...>;
    using pointer = pointer_t;
    using reference = value_type;

    // iterator(Types* ... ptrs);
    // iterator(typename TypedMeshAttributeBuffer<Types>::iterator ... iters);
    iterator(uint32_t index, TypedMeshAttributeBuffer<Types>& ... buffers);

    reference operator*();

    pointer operator->();

    iterator& operator++();

    iterator operator++(int);

    bool operator==(const iterator& other) const;

    bool operator!=(const iterator& other) const;

private:
    // std::tuple<Types* ...> _ptrs;
    // std::tuple<Types& ...> _refs;
    // std::tuple<typename TypedMeshAttributeBuffer<Types>::iterator ...> _iters;
    uint32_t _index;
    const std::tuple<TypedMeshAttributeBuffer<Types>& ...>  _buffers;
};

template<typename ... Types>
class MeshAttributeView<Types...>::iterator::pointer_t {
public:
    pointer_t(value_type&& v) :
        _value(std::move(v)) {
    }

    pointer_t(const pointer_t&) = delete;

    pointer_t(pointer_t&&) = default;

    pointer_t& operator=(const pointer_t&) = delete;

    pointer_t& operator=(pointer_t&&) = default;

    value_type* operator->() {
        return std::addressof(_value);
    }

    value_type& operator*() {
        return *operator->();
    }

private:
    value_type _value;
};

template<typename ... Types>
MeshAttributeView<Types...>::MeshAttributeView(TypedMeshAttributeBuffer<Types>& ... buffers) :
        _buffers(buffers...) {
}

template<typename ... Types>
typename MeshAttributeView<Types...>::iterator MeshAttributeView<Types...>::begin() {
    // return std::apply([] (auto ... args) { return iterator((args.begin())...); }, _buffers);
    return std::apply([] (auto& ... args) { return iterator(0, args...); }, _buffers);
}

template<typename ... Types>
typename MeshAttributeView<Types...>::iterator MeshAttributeView<Types...>::end() {
    // return std::apply([] (auto ... args) { return iterator((args.end())...); }, _buffers);
    auto num = std::get<0>(_buffers).elements().size();
    return std::apply([n = num] (auto& ... args) { return iterator(n, args...); }, _buffers);
}

// template<typename ... Types>
// MeshAttributeView<Types...>::iterator::iterator(Types* ... ptrs) :
//         _ptrs(ptrs...),
//         _refs(*ptrs...) {
//     std::cout << "Constructing iterator:" << std::endl;
//     ((std::cout << vvm::to_string(*ptrs) << std::endl), ...);
// }


// template<typename ... Types>
// MeshAttributeView<Types...>::iterator::iterator(typename TypedMeshAttributeBuffer<Types>::iterator ... iters) :
//         _iters(iters...) {
//     std::cout << "Constructing iterator:" << std::endl;
//     ((std::cout << vvm::to_string(*iters) << std::endl), ...);
// }

template<typename ... Types>
MeshAttributeView<Types...>::iterator::iterator(uint32_t index, TypedMeshAttributeBuffer<Types>& ... buffers) :
        _index(index),
        _buffers(buffers...) {
}

template<typename ... Types>
typename MeshAttributeView<Types...>::iterator::reference MeshAttributeView<Types...>::iterator::operator*() {
    // return _refs;
    // return std::apply([] (auto... args) { return std::tuple_cat(std::forward_as_tuple((*args))...); }, _iters);
    return std::apply([i = _index] (auto& ... args) { return std::forward_as_tuple(args[i]...); }, _buffers);
}

template<typename ... Types>
typename MeshAttributeView<Types...>::iterator::pointer MeshAttributeView<Types...>::iterator::operator->() {
    return pointer_t(operator*());
}

template<typename ... Types>
typename MeshAttributeView<Types...>::iterator& MeshAttributeView<Types...>::iterator::operator++() {
    // https://stackoverflow.com/questions/16387354/template-tuple-calling-a-function-on-each-element/37100197#37100197
    // _ptrs = std::apply([] (auto... ptrs) { return std::make_tuple(++ ptrs ...); }, _ptrs);
    // _refs = std::apply([] (auto... ptrs) { return std::make_tuple(* ptrs ...); }, _ptrs);
    // _iters = std::apply([] (auto ... iters) { return std::make_tuple((++iters)...); }, _iters);
    ++_index;
    return *this;
}

template<typename ... Types>
typename MeshAttributeView<Types...>::iterator MeshAttributeView<Types...>::iterator::operator++(int) {
    iterator temp = *this;
    operator++();
    return temp;
}

namespace detail {

template<typename ... Types, size_t ... Inds>
bool cmp(const std::tuple<Types ...>& first, const std::tuple<Types ...>& second, const std::integer_sequence<size_t, Inds...>& is) {
    return ((std::get<Inds>(first) == std::get<Inds>(second)) && ...);
}

}

template<typename ... Types>
bool MeshAttributeView<Types...>::iterator::operator==(const iterator &other) const {
    static constexpr auto seq = std::index_sequence_for<Types...>();
    auto to_ptrs = [] (const auto& t) {
        return std::apply([] (auto&... args) {
            return std::make_tuple((&args)...); }, t); };
    return _index == other._index && detail::cmp(to_ptrs(_buffers), to_ptrs(other._buffers), seq);
}

template<typename ... Types>
bool MeshAttributeView<Types...>::iterator::operator!=(const iterator &other) const {
    return !operator==(other);
}