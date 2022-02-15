#pragma once

#include <iostream>
#include <tuple>

#include <vvm/string.hpp>

#include "attribute_buffer.hpp"



template<typename ... Buffers>
class MeshAttributeView {

public:

    class iterator;

    MeshAttributeView(Buffers& ... buffers);

    iterator begin();

    iterator end();

private:

    std::tuple<Buffers& ...> _buffers;

};

namespace detail {

template<typename T1, typename T2, typename Enable = void>
struct conditional_const;

template<typename T1, typename T2>
struct conditional_const<T1, T2, typename std::enable_if_t<!std::is_const_v<T1>>> {
    using type = std::remove_const_t<T2>;
};

template<typename T1, typename T2>
struct conditional_const<T1, T2, typename std::enable_if_t<std::is_const_v<T1>>> {
    using type = std::add_const_t<T2>;
};

template<typename T1, typename T2>
using conditional_const_t = typename conditional_const<T1, T2>::type;

}

template<typename ... Buffers>
class MeshAttributeView<Buffers...>::iterator {
public:
    class pointer_t;

    // this could be implemented as a random access iterator, since it's just using an index into buffers
    // but forward iterator is really all we need for the use case of just iterating the vertices, which is why this header exists
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    // conditional_const uses add_const/remove_const, which in turn means don't pass the reference type as T2, because
    // reference to const is not actually a const type, instead pass the base type and put the reference outside
    using value_type = std::tuple<detail::conditional_const_t<Buffers, typename Buffers::value_type>&...>;
    using pointer = pointer_t;
    using reference = value_type;

    iterator(uint32_t index, Buffers& ... buffers);

    reference operator*();

    pointer operator->();

    iterator& operator++();

    iterator operator++(int);

    bool operator==(const iterator& other) const;

    bool operator!=(const iterator& other) const;

private:
    uint32_t _index;
    const std::tuple<Buffers& ...>  _buffers;
};

// wrapper around a value, so we don't take a pointer to an rvalue
template<typename ... Buffers>
class MeshAttributeView<Buffers...>::iterator::pointer_t {
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

template<typename ... Buffers>
MeshAttributeView<Buffers...>::MeshAttributeView(Buffers& ... buffers) :
        _buffers(buffers...) {
}

template<typename ... Buffers>
typename MeshAttributeView<Buffers...>::iterator MeshAttributeView<Buffers...>::begin() {
    return std::apply([] (auto& ... args) { return iterator(0, args...); }, _buffers);
}

template<typename ... Buffers>
typename MeshAttributeView<Buffers...>::iterator MeshAttributeView<Buffers...>::end() {
    auto num = std::get<0>(_buffers).elements().size();
    return std::apply([n = num] (auto& ... args) { return iterator(n, args...); }, _buffers);
}

template<typename ... Buffers>
MeshAttributeView<Buffers...>::iterator::iterator(uint32_t index, Buffers& ... buffers) :
        _index(index),
        _buffers(buffers...) {
}

template<typename ... Buffers>
typename MeshAttributeView<Buffers...>::iterator::reference MeshAttributeView<Buffers...>::iterator::operator*() {
    return std::apply([i = _index] (auto& ... args) { return std::forward_as_tuple(args[i]...); }, _buffers);
}

template<typename ... Buffers>
typename MeshAttributeView<Buffers...>::iterator::pointer MeshAttributeView<Buffers...>::iterator::operator->() {
    return pointer_t(operator*());
}

template<typename ... Buffers>
typename MeshAttributeView<Buffers...>::iterator& MeshAttributeView<Buffers...>::iterator::operator++() {
    ++_index;
    return *this;
}

template<typename ... Buffers>
typename MeshAttributeView<Buffers...>::iterator MeshAttributeView<Buffers...>::iterator::operator++(int) {
    iterator temp = *this;
    operator++();
    return temp;
}

template<typename ... Buffers>
bool MeshAttributeView<Buffers...>::iterator::operator==(const iterator &other) const {
    auto to_ptrs = [] (const auto& t) {
        return std::apply([] (auto&... args) { return std::make_tuple((&args)...); }, t);
    };
    auto cmp_ptrs = [] (const auto& t1, const auto& t2) {
        return std::apply([&t2] (const auto& ... args1) {
            return std::apply([&args1...] (const auto& ... args2) {
                return ((args1 == args2) && ...);
            }, t2);
        }, t1);
    };
    
    return _index == other._index && cmp_ptrs(to_ptrs(_buffers), to_ptrs(other._buffers));
}

template<typename ... Buffers>
bool MeshAttributeView<Buffers...>::iterator::operator!=(const iterator &other) const {
    return !operator==(other);
}