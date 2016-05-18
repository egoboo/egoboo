#pragma once

#define EGOLIB_MATH_PRIVATE 1

#include "egolib/Math/Angle.hpp"
#include "egolib/Math/AABB.hpp"
#include "egolib/Math/Math.hpp"
#include "egolib/Math/ColourRgb.hpp"
#include "egolib/Math/ColourRgba.hpp"
#include "egolib/Math/ColourL.hpp"
#include "egolib/Math/ColourLa.hpp"
#include "egolib/Math/Cone3.hpp"
#include "egolib/Math/Convex.hpp"
#include "egolib/Math/Cube.hpp"
#include "egolib/Math/LERP.hpp"
#include "egolib/Math/Line.hpp"
#include "egolib/Math/Matrix.hpp"
#include "egolib/Math/Point.hpp"
#include "egolib/Math/Plane.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Math/Ray.hpp"
#include "egolib/Math/Sphere.hpp"
#include "egolib/Math/Standard.hpp"
#include "egolib/Math/Transform.hpp"
#include "egolib/Math/Vector.hpp"
#include "egolib/Math/VectorProjection.hpp"
#include "egolib/Math/VectorRejection.hpp"


#include "egolib/Math/Contains_AABB_AABB.hpp"
#include "egolib/Math/Contains_Cube_Cube.hpp"
#include "egolib/Math/Contains_Point_Point.hpp"
#include "egolib/Math/Contains_Sphere_Sphere.hpp"
//
#include "egolib/Math/Contains_Sphere_Point.hpp"
//
#include "egolib/Math/Contains_AABB_Point.hpp"


#include "egolib/Math/Intersects_AABB_AABB.hpp"
#include "egolib/Math/Intersects_Cube_Cube.hpp"
#include "egolib/Math/Intersects_Point_Point.hpp"
#include "egolib/Math/Intersects_Sphere_Sphere.hpp"
//
#include "egolib/Math/Intersects_Sphere_Point.hpp"
#include "egolib/Math/Intersects_Point_Sphere.hpp"
//
#include "egolib/Math/Intersects_AABB_Point.hpp"
#include "egolib/Math/Intersects_Point_AABB.hpp"
//
#include "egolib/Math/Intersects_AABB_Cube.hpp"
#include "egolib/Math/Intersects_Cube_AABB.hpp"
//
#include "egolib/Math/Intersects_Cube_Point.hpp"
#include "egolib/Math/Intersects_Point_Cube.hpp"

#include "egolib/Math/Discrete.hpp"

#undef EGOLIB_MATH_PRIVATE
