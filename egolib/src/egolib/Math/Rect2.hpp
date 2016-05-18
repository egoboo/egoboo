//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file  egolib/Math/Rect2.hpp
/// @brief 2D rectangle.

#include "egolib/Math/EuclideanSpace.hpp"

namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType, typename _EnabledType = std::enable_if_t<_EuclideanSpaceType::dimensionality() == 2>>
class Rect2 : public Translatable<typename _EuclideanSpaceType::VectorSpaceType> {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(Rect2);
private:
    PointType min, max;
public:
    /**
     * @brief Construct this rectangle with its default values.
     * @remark The default values of a rectangle are
     * \f$min = \left(0,0\ight)\f$ and \f$max = \left(0,0\right)\f$.
     */
    Rect2() : min(), max() {}
    /**
     * @brief Construct this rectangle with specified points.
     * @param a, b the points
     * @remark The values of the rectangle are
     * \f$min = \left(\min(a_x,b_x),\min(a_y,b_y)\right)\f$ and \f$max = \left(\max\left(a_x,b_x\right),\max\left(a_y,b_y\right)\right)\f$.
     */
    Rect2(const PointType& a, const PointType& b) : min(a), max(b) {
        if (min.x() > max.x()) std::swap(min.x(), max.x());
        if (min.y() > max.y()) std::swap(min.y(), max.y());
    }
    /**
     * @brief Construct this rectangle with another rectangle.
     * @param other the other rectangle
     */
    Rect2(const MyType& other) : min(other.min), max(other.max) {}
    /**
     * @brief Get the minimum point of this rectangle.
     * @return the minimum point of this rectangle
     */
    const PointType& getMin() const {
        return min;
    }
    /**
     * @brief Get the maximum point of this rectangle.
     * @return the maximum point of this rectangle
     */
    const PointType& getMax() const {
        return max;
    }
public:
    /** @copydoc Ego::Math::Translatable */
    void translate(const VectorType& t) override {
        min += t;
        max += t;
    }
};

} // namespace Math
} // namespace Ego
