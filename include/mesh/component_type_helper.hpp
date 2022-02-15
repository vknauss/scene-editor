#pragma once

#include "vector_math.hpp"

#include "mesh/attribute.hpp"


// Helper type for TypedMeshAttributeBuffer to get its component info

template<typename TT>
struct ComponentTypeHelper;

template<>
struct ComponentTypeHelper<float> {
    static constexpr MeshAttributeComponentType componentType = MeshAttributeComponentType::FLOAT;
    static constexpr int numComponents = 1;
};

template<>
struct ComponentTypeHelper<int> {
    static constexpr MeshAttributeComponentType componentType = MeshAttributeComponentType::INT;
    static constexpr int numComponents = 1;
};

template<>
struct ComponentTypeHelper<unsigned int> {
    static constexpr MeshAttributeComponentType componentType = MeshAttributeComponentType::UINT;
    static constexpr int numComponents = 1;
};

template<int D>
struct ComponentTypeHelper<vecmath::vector<float, D>> {
    static constexpr MeshAttributeComponentType componentType = MeshAttributeComponentType::FLOAT;
    static constexpr int numComponents = D;
};

template<int D>
struct ComponentTypeHelper<vecmath::vector<int, D>> {
    static constexpr MeshAttributeComponentType componentType = MeshAttributeComponentType::INT;
    static constexpr int numComponents = D;
};

template<int D>
struct ComponentTypeHelper<vecmath::vector<unsigned int, D>> {
    static constexpr MeshAttributeComponentType componentType = MeshAttributeComponentType::UINT;
    static constexpr int numComponents = D;
};