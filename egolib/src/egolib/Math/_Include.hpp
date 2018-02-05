#pragma once

#define EGOLIB_MATH_PRIVATE 1

//
#include "egolib/Math/Matrix.hpp"

//
#include "egolib/Math/Cone3.hpp"

//
#include "egolib/Math/Math.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Math/Standard.hpp"
#include "egolib/Math/Transform.hpp"
#include "egolib/Math/VectorProjection.hpp"
#include "egolib/Math/VectorRejection.hpp"

//
#include "egolib/Math/Functors/is_intersecting_axis_aligned_box_axis_aligned_cube.hpp"

//
#include "egolib/Math/Functors/enclose_axis_aligned_cube_in_axis_aligned_box.hpp"
#include "egolib/Math/Functors/enclose_sphere_in_axis_aligned_box.hpp"
#include "egolib/Math/Functors/enclose_axis_aligned_box_in_axis_aligned_cube.hpp"
#include "egolib/Math/Functors/enclose_axis_aligned_box_in_sphere.hpp"

#undef EGOLIB_MATH_PRIVATE
