#pragma once

#include <cstdint>


enum class MeshAttribute {
    POSITION, NORMAL, COLOR, TEXCOORD, BONE_INDS, BONE_WEIGHTS
};

enum class MeshAttributeComponentType : uint8_t {
    FLOAT = 0,
    INT = 1,
    UINT = 2
};

inline constexpr const char* attributeName(MeshAttribute attribute) {
    switch(attribute) {
    case MeshAttribute::POSITION:
        return "Position";
    case MeshAttribute::NORMAL:
        return "Normal";
    case MeshAttribute::COLOR:
        return "Color";
    case MeshAttribute::TEXCOORD:
        return "Texture coordinate";
    case MeshAttribute::BONE_INDS:
        return "Bone indices";
    case MeshAttribute::BONE_WEIGHTS:
        return "Bone weights";
    default:
        return "Unknown";
    }
}

// inline constexpr size_t attributeSize(MeshAttribute attribute) {

// }