#pragma once

#include "gtest/gtest.h"
#include "egolib/egolib.h"

namespace ego { namespace math { namespace test {

template <typename Scalar, size_t Dimensionality>
id::vector<Scalar, Dimensionality> normalize(const id::vector<Scalar, Dimensionality>& v) {
    auto l = id::euclidean_norm(v);
    if (l < id::zero<Scalar>()) {
        throw id::runtime_error(__FILE__, __LINE__, "negative length");
    } else {
        return v * (1.0f / l);
    }
}

struct Utilities {
    /// \f$\delta^+(x)\f$ is a value greater than \f$x\f$
    /// such that \f$|x-\delta(x)|\f$ is sufficient that
    /// \f$f(x) \neq f(\delta(x))\f$ if \f$f\f$ is
    /// a function.
    static float pdelta(float x) {
        return x + 0.001f;
    }
    /// \f$\delta^+(x)\f$ is a value smaller than \f$x\f$
    /// such that \f$|x-\delta(x)|\f$ is sufficient that
    /// \f$f(x) \neq f(\delta(x))\f$ if \f$f\f$ is
    /// a function.
    static float ndelta(float x) {
        return x - 0.001f;
    }

    // The unit vector.
    static Vector3f unit() {
        static const auto v = normalize(Vector3f(1.0f, 1.0f, 1.0f));
        return v;
    }

public:
    /**
     * Get a point which does not intersect with the specified point.
     *
     * Let \f$a\f$ be the specified point.
     * The point
     * \f$b = a + \delta^+(1) \vec{1}\f$
     * does not intersect with \f$a\f$.
     */
    static Point3f getNonOverlappingPoint3f(const Point3f& a) {
        return a + unit() * pdelta(1.0f);
    }
    /**
     * Get a point which overlaps with the specified point.
     *
     * Let \f$a\f$ be the specified point.
     * The point
     * \f$b = a\f$
     * overlaps with \f$a\f$.
     */
    static Point3f getOverlappingPoint3f(const Point3f& a) {
        return a;
    }
    /**
     * Get a point which is not contained in the specified point.
     *
     * Let \f$a\f$ be the specified point.
     * The point
     * \f$b = a + \epsilon\vec{1}\f$
     * is not contained in \$a\f$.
     */
    static Point3f getNonContainedPoint3f(const Point3f& a) {
        return a + unit() * pdelta(0.0f);
    }
    // Get a point which is contained in the specified point.
    //
    // Let \f$a\f$ be the specified point.
    // The point
    // \f$b = a\f$
    // is contained in \f$a\f$.
    static Point3f getContainedPoint3f(const Point3f& a) {
        return a;
    }

public:
    // Get a sphere which does not intersect with the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The sphere \f$b\f$ with
    // center \f$\mathbf{c}' = \mathbf{c} + \delta^+(2r)\vec{1}\f$ and radius \f$r'=r\f$
    // does not intersect with \f$a\f$.
    static Sphere3f getNonOverlappingSphere3f(const Sphere3f& a) {
        auto c = a.getCenter() + unit() * pdelta(2.0f * a.getRadius());
        auto r = a.getRadius();
        return Sphere3f(c, r);
    }

    // Get a point which does not intersect with the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The point
    // \f$b = c + \delta^+(r) \vec{1}\f$
    // does not intersect \f$a\f$.
    static Point3f getNonOverlappingPoint3f(const Sphere3f& a) {
        return a.getCenter() + unit() * pdelta(a.getRadius());
    }

    // Get a sphere which intersects with the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The sphere \f$b\f$ with center
    // center \f$c' = c + r \vec{1}\f$ and radius \f$r\f$
    // intersects with \f$a\f$.
    static Sphere3f getOverlappingSphere3f(const Sphere3f& a) {
        return Sphere3f(a.getCenter() + unit() * a.getRadius(), a.getRadius());
    }
    // Get a point which intersects with the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The point
    // \f$b = c + ndelta(r) \vec{1}\f$
    // intersects with \f$a\f$.
    static Point3f getOverlappingPoint3f(const Sphere3f& a) {
        return a.getCenter() + unit() * ndelta(a.getRadius());
    }

    // Get a point which is not contained in the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The point
    // \f$b = c + \delta^+(r) \vec{1}\f$
    // does not intersect with \f$a\f$.
    static Point3f getNonContainedPoint3f(const Sphere3f& a) {
        return a.getCenter() + unit() * pdelta(a.getRadius());
    }
    // Get a sphere which is not contained in the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The sphere \f$b\f$ with
    // center \f$c' = c + \delta^+(r) \vec{1}\f$ and radius \f$r' = r\f$
    // is not contained in \f$a\f$.
    static Sphere3f getNonContainedSphere3f(const Sphere3f& a) {
        return Sphere3f(a.getCenter() + unit() * pdelta(a.getRadius()), a.getRadius());
    }

    // Get a point which is contained in the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The point
    // \f$b = c + \delta^-(r) \vec{1}\f$
    // intersects with \f$a\f$.
    static Point3f getContainedPoint3f(const Sphere3f& a) {
        return a.getCenter() + unit() * ndelta(a.getRadius());
    }
    // Get a sphere which is contained in the specified sphere.
    //
    // Let \f$a\f$ be the specified sphere with center \f$c\f$ and radius \f$r\f$.
    // The sphere \f$b\f$ with
    // center \f$c' = c + \frac{1}{2}r\vec{1}\f$ and radius \f$r' = \frac{1}{2}r\f$
    // is contained in \f$a\f$.
    static Sphere3f getContainedSphere3f(const Sphere3f& a) {
        return Sphere3f(a.getCenter() + unit() * (0.5 * a.getRadius()), 0.5 * a.getRadius());
    }

public:
    // Get an AABB which does not overlap with the specified AABB.
    //
    // Let \f$a\f$ be the specified axis-aligned bounding box with
    // minimum point \f$\mathbf{m}\f$ and edge lengths \f$\vec{l}\f$.
    // The axis-aligned bounding box \f$b\f$ with minimum point
    // \f$\mathbf{m}' = \mathbf{m} + \delta^+(0)\vec{1} + \vec{l}\f$
    // and edge lengths \f$\vec{l}' = \vec{l}\f$ does not overlap with \f$a\f$.
    static AxisAlignedBox3f getNonOverlappingAxisAlignedBox3f(const AxisAlignedBox3f& x) {
        auto t = pdelta(0.0f);
        auto d = unit() * t + x.getSize();
        return AxisAlignedBox3f(x.getMin() + d, x.getMax() + d);
    }

    // Get an axis-aligned bounding box which intersects with the specified axis-aligned bounding box.
    //
    // Let \f$a\f$ be the specified axis-aligned bounding box with minimum point \f$m\f$
    // and edge lengths \f$\vec{l}\f$. The axis-aligned bounding box \f$b\f$ with
    // minimum point \f$m' = m + \frac{1}{4}\vec{l}\f$ and edge lengths \f$\vec{l}\f$ intersects with \f$a\f$.
    static AxisAlignedBox3f getOverlappingAxisAlignedBox3f(const AxisAlignedBox3f& a) {
        auto d = a.getSize() * 0.25f;
        return AxisAlignedBox3f(a.getMin() + d, a.getMax() + d);
    }

    // Get a point which overlaps with the specified AABB.
    static Point3f getOverlappingPoint3f(const AxisAlignedBox3f& x) {
        return x.getCenter();
    }

    // Get a point which does not overlap with the specified axis-aligned bounding box.
    //
    // Let \f$a\f$ be the specified axis-aligned bounding box with minimum point \f$m\f$
    // and edge lengths \f$\vec{l}\f$. The point \f$b = min + l + \delta^+(0) \vec{1}\f$
    // does not intersect \f$a\f$.
    static Point3f getNonOverlappingPoint3f(const AxisAlignedBox3f& a) {
        return a.getMin() + a.getSize() + unit() * pdelta(0.0f);
    }

    // Get an axis-aligned bounding box which is not contained in the specified axis-aligned bounding box.
    //
    // Let \f$a\f$ be the specified axis-aligned bounding box with minimum point \f$m\f$
    // and edge lengths \f$\vec{l}\f$. The axis-aligned bounding box \f$b\f$ with
    // minimum point \f$m' = m + \frac{1}{4}\vec{l}\f$ and edge lengths \f$\vec{l}\f$ is not contained in \f$a\f$.
    static AxisAlignedBox3f getNonContainedAxisAlignedBox3f(const AxisAlignedBox3f& a) {
        auto d = a.getSize() * 0.25f;
        return AxisAlignedBox3f(a.getMin() + d, a.getMax() + d);
    }

    // Get a point which is not contained in the specified axis-aligned bounding box.
    //
    // Let \f$a\f$ be the specified axis-aligned bounding box.
    // The point
    // \f$b = a_{max} + \delta^+(0)\vec{1}\f$
    // is not contained in \f$a\f$.
    static Point3f getNonContainedPoint3f(const AxisAlignedBox3f& a) {
        return a.getMax() + unit() * pdelta(0.0f);
    }

    // Get an AABB which is contained in the specified AABB.
    // Let \f$a\f$ be the specified axis-aligned bounding box
    // with minimum \f$min\f$ and maximum \f$max\f$.
    // The axis-aligned bounding box \f$b\f$ with
    // minimum \f$min' = (min - c) \delta^-(1) + c\f$
    // and
    // maximum \f$max' = (max - c) \delta^-(1) + c\f$
    // is contained in \f$a\f$.
    static AxisAlignedBox3f getContainedAxisAlignedBox3f(const AxisAlignedBox3f& a) {
        auto s = ndelta(1.0f);
        auto min = a.getCenter() + (a.getMin() - a.getCenter())*s;
        auto max = a.getCenter() + (a.getMax() - a.getCenter())*s;
        return AxisAlignedBox3f(min, max);
    }

    // Get a point which is contained in the specified axis-aligned bounding box.
    // Let \f$a\f$ be the specified axis-aligned bounding box with center \f$c\f$.
    // The point \f$b = c\f$ is contained in \f$a\f$.
    static Point3f getContainedPoint3f(const AxisAlignedBox3f& a) {
        return a.getCenter();
    }

public:
    // Get a cube which does not overlap with the specified cube.
    // 
    // Let \f$a\f$ be the specified cube with
    // minimum point \f$\mathbf{m}\f$ and edge length \f$l\f$.
    // The cube \f$b\f$ with minimum point
    // \f$\mathbf{m}' = \mathbf{m} + \delta^+(l)\vec{1}\f$
    // and edge length \f$l' = l\f$ does not overlap with \f$a\f$.
    static AxisAlignedCube3f getNonOverlappingAxisAlignedCube3f(const AxisAlignedCube3f& a) {
        auto d = Vector3f(1.0f, 1.0f, 1.0f) * pdelta(a.getSize());
        return AxisAlignedCube3f(a.getCenter() + d, a.getSize());
    }

    // Get an axis-aligned cube which intersects with the specified axis-aligned cube.
    //
    // Let \f$a\f$ be the specified axis-aligned cube with minimum point \f$m\f$
    // and edge length \f$l\f$. The axis-aligned bounding cube \f$b\f$ with
    // minimum point \f$m' = m + \frac{1}{4}l\vec{1}\f$ and edge length \f$l' = l\f$
    // intersects with \f$a\f$.
    static AxisAlignedCube3f getOverlappingAxisAlignedCube3f(const AxisAlignedCube3f& a) {
        auto d = unit() * (a.getSize() * 0.25f);
        return AxisAlignedCube3f(a.getMin() + d, a.getSize());
    }

    // Get a point which overlaps with the specified axis-aligned cube.
    static Point3f getOverlappingPoint3f(const AxisAlignedCube3f& a) {
        return a.getCenter();
    }

    // Get a point which does not overlap with the specified axis-aligned cube.
    //
    // Let \f$a\f$ be the specified axis-aligned cube with minimum point \f$m\f$
    // and edge length \f$l\f$. The point \f$b = min + \delta^+(l) \vec{1}\f$
    // does not intersect \f$a\f$.
    static Point3f getNonOverlappingPoint3f(const AxisAlignedCube3f& a) {
        return a.getMin() + unit() * a.getSize() + unit() * pdelta(a.getSize());
    }

    // Get a point which is not contained in the specified axis-aligned cube.
    //
    // Let \f$a\f$ be the specified axis-aligned cube.
    // The point
    // \f$b = a_{max} + \delta^+(0.0f)\vec{1}\f$
    // is not contained in \f$a\f$.
    static Point3f getNonContainedPoint3f(const AxisAlignedCube3f& a) {
        return a.getMax() + unit() * pdelta(0.0f);
    }

	// Add basis vectors (1,0,0), (0,1,0), (0,0,1).
	static std::vector<Vector3f> basis(std::vector<Vector3f> l) {
		std::vector<Vector3f> l1{ l };
		l1.emplace_back(1.0f, 0.0f, 0.0f);
		l1.emplace_back(0.0f, 1.0f, 0.0f);
		l1.emplace_back(0.0f, 0.0f, 1.0f);
		return l1;
	}

	// Add the zero vector to a list.
	static std::vector<Vector3f> zero(std::vector<Vector3f> l) {
		std::vector<Vector3f> l1{ l };
		l1.emplace_back(0.0f, 0.0f, 0.0f);
		return l1;
	}

	// Add negation of all list elements to a list.
	template <typename T>
	static std::vector<T> negation(std::vector<T> l) {
		std::vector<T> l1{ l };
		for (auto e : l) {
			auto e1 = -e;
			l1.emplace_back(-e1);
		}
		return l1;
	}

	// Cartesian product list of two list.
	template <typename T>
	static std::vector<std::pair<T, T>> cartesian(const std::vector<T>& a, const std::vector<T>& b) {
		std::vector<std::pair<T, T>> c;
		for (auto x : a) {
			for (auto y : b) {
				c.emplace_back(x, y);
			}
		}
		return c;
	}

};

} } } // namespace ego::math::test
