#pragma once

// If you would like to use this framework with a different vector math library,
// this header is intended to be the only place you need to change the core
// systems.
// Vector math types and operations should be aliased/implemented here, 
// or linked in via includes in this file.

#include <vvm/vvm.hpp>
#include <vvm/matrix_tfm.hpp>

namespace vecmath = vvm;

using vec2 = vecmath::vector<float, 2>;
using vec3 = vecmath::vector<float, 3>;
using vec4 = vecmath::vector<float, 4>;

using mat2 = vecmath::matrix<float, 2, 2>;
using mat3 = vecmath::matrix<float, 3, 3>;
using mat4 = vecmath::matrix<float, 4, 4>;

using ivec2 = vecmath::vector<int, 2>;
using ivec3 = vecmath::vector<int, 3>;
using ivec4 = vecmath::vector<int, 4>;

using uvec2 = vecmath::vector<unsigned int, 2>;
using uvec3 = vecmath::vector<unsigned int, 3>;
using uvec4 = vecmath::vector<unsigned int, 4>;
