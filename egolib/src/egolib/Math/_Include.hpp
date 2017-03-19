#pragma once

#define EGOLIB_MATH_PRIVATE 1

//
#include "egolib/Math/Interval.hpp"

//
#include "egolib/Math/Angle.hpp"
#include "egolib/Math/Point.hpp"
#include "egolib/Math/Matrix.hpp"
#include "egolib/Math/Vector.hpp"

//
#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/Cone3.hpp"
#include "egolib/Math/AxisAlignedCube.hpp"
#include "egolib/Math/Line.hpp"
#include "egolib/Math/Plane.hpp"
#include "egolib/Math/Ray.hpp"
#include "egolib/Math/Sphere.hpp"

//
#include "egolib/Math/Math.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Math/Standard.hpp"
#include "egolib/Math/Transform.hpp"
#include "egolib/Math/VectorProjection.hpp"
#include "egolib/Math/VectorRejection.hpp"

//
#include "egolib/Math/Functors/Interpolate.hpp"
#include "egolib/Math/Functors/Interpolate_Linear_Point.hpp"
#include "egolib/Math/Functors/Interpolate_Linear_Real.hpp"
#include "egolib/Math/Functors/Interpolate_Linear_Vector.hpp"
#include "egolib/Math/Functors/Interpolate_Linear_ColourRGB.hpp"
#include "egolib/Math/Functors/Interpolate_Linear_ColourRGBA.hpp"

//
#include "idlib/math/even_odd.hpp"
#include "idlib/math/one_zero.hpp"

//
#include "egolib/Math/Functors/Contains_AxisAlignedBox_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Contains_AxisAlignedCube_AxisAlignedCube.hpp"
#include "egolib/Math/Functors/Contains_Point_Point.hpp"
#include "egolib/Math/Functors/Contains_Sphere_Sphere.hpp"
#include "egolib/Math/Functors/Contains_Sphere_Point.hpp"
#include "egolib/Math/Functors/Contains_AxisAlignedBox_Point.hpp"

//
#include "egolib/Math/Functors/Intersects_AxisAlignedBox_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Intersects_AxisAlignedCube_AxisAlignedCube.hpp"
#include "egolib/Math/Functors/Intersects_Point_Point.hpp"
#include "egolib/Math/Functors/Intersects_Sphere_Sphere.hpp"
#include "egolib/Math/Functors/Intersects_Sphere_Point.hpp"
#include "egolib/Math/Functors/Intersects_Point_Sphere.hpp"
#include "egolib/Math/Functors/Intersects_AxisAlignedBox_Point.hpp"
#include "egolib/Math/Functors/Intersects_Point_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Intersects_AxisAlignedBox_AxisAlignedCube.hpp"
#include "egolib/Math/Functors/Intersects_AxisAlignedCube_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Intersects_AxisAlignedCube_Point.hpp"
#include "egolib/Math/Functors/Intersects_Point_AxisAlignedCube.hpp"

//
#include "egolib/Math/Functors/Translate_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Translate_Cone3.hpp"
#include "egolib/Math/Functors/Translate_AxisAlignedCube.hpp"
#include "egolib/Math/Functors/Translate_Line.hpp"
#include "egolib/Math/Functors/Translate_Plane.hpp"
#include "egolib/Math/Functors/Translate_Point.hpp"
#include "egolib/Math/Functors/Translate_Ray.hpp"
#include "egolib/Math/Functors/Translate_Sphere.hpp"

//
#include "egolib/Math/Functors/Convert_Lf_Lf.hpp"
#include "egolib/Math/Functors/Convert_LAf_LAf.hpp"
#include "egolib/Math/Functors/Convert_Lf_LAf.hpp"
#include "egolib/Math/Functors/Convert_LAf_Lf.hpp"

#include "egolib/Math/Functors/Convert_RGBf_RGBf.hpp"
#include "egolib/Math/Functors/Convert_RGBAf_RGBAf.hpp"
#include "egolib/Math/Functors/Convert_RGBf_RGBAf.hpp"
#include "egolib/Math/Functors/Convert_RGBAf_RGBf.hpp"

//
#include "egolib/Math/Functors/Closure_AxisAlignedBox_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Closure_AxisAlignedBox_AxisAlignedCube.hpp"
#include "egolib/Math/Functors/Closure_AxisAlignedBox_Sphere.hpp"
#include "egolib/Math/Functors/Closure_AxisAlignedCube_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Closure_AxisAlignedCube_AxisAlignedCube.hpp"
#include "egolib/Math/Functors/Closure_Sphere_AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Closure_Sphere_Sphere.hpp"

//
#include "egolib/Math/Discrete.hpp"

#undef EGOLIB_MATH_PRIVATE
