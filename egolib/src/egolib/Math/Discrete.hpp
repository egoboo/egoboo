#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {
namespace Discrete {

template <typename T, typename Enabled = void>
struct Size2;

template <typename T>
struct Size2<T, std::enable_if_t<std::is_same<T, int>::value>>
    : public id::equal_to_expr<Size2<T>> {
private:
    using MyType = Size2<T>;
    std::array<T, 2> elements;

public:
    Size2() : elements{0,0} {}
    Size2(T width, T height) : elements{width,height} {}
    Size2(const MyType& other) : elements(other.elements) {}

public:
    // CRTP
    bool equal_to(const MyType& other) const
    {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

public:
    T& width() {
        return elements[0];
    }
    const T& width() const {
        return elements[0];
    }
    T& height() {
        return elements[1];
    }
    const T& height() const {
        return elements[1];
    }

}; // struct Size2

template <typename T, typename Enabled = void>
struct Vector2;

template <typename T>
struct Vector2<T, std::enable_if_t<std::is_same<T, int>::value>>
    : public Internal::VectorExpr<Vector2<T>, T>
{
private:
    using MyType = Vector2<T>;
    std::array<T, 2> elements;

public:
    Vector2() : elements{0,0} {}
    Vector2(T x, T y) : elements{x,y} {}
    Vector2(const Vector2<T>& other) : elements(other.elements) {}

public:
    const MyType& operator=(const MyType& other) {
        elements = other.elements;
        return *this;
    }

public:
    // CRTP
    void add(const MyType& other)
    {
        elements[0] += other.elements[0];
        elements[1] += other.elements[1];
    }

    // CRTP
    void subtract(const MyType& other)
    {
        elements[0] -= other.elements[0];
        elements[1] -= other.elements[1];
    }

    // CRTP
    bool equal_to(const MyType& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

    // CRTP
    MyType unary_plus() const
    {
        return MyType(+elements[0], +elements[1]);
    }
    
    // CRTP
    MyType unary_minus() const
    {
        return MyType(-elements[0], -elements[1]);
    }

    // CRTP
    void multiply(const T& scalar)
    {
        elements[0] *= scalar;
        elements[1] *= scalar;
    }

    // CRTP
    void divide(const T& scalar)
    {
        elements[0] /= scalar;
        elements[1] /= scalar;
    }

public:
    T& x()
    {
        return elements[0];
    }
    
    const T& x() const
    {
        return elements[0];
    }
    
    T& y()
    {
        return elements[1];
    }

    const T& y() const
    {
        return elements[1];
    }

}; // struct Vector2

template <typename T, typename Enabled = void>
struct Point2;

template <typename T>
struct Point2<T, std::enable_if_t<std::is_same<T, int>::value>>
    : public Internal::PointExpr<Point2<T>, Vector2<T>> {
private:
    using VectorType = Vector2<T>;
    using MyType = Point2<T>;
    std::array<T, 2> elements;

public:
    Point2() : elements{0,0} {}
    Point2(T x, T y) : elements{x,y} {}
    Point2(const MyType& other) : elements(other.elements) {}

public:
    const MyType& operator=(const MyType& other) {
        elements = other.elements;
        return *this;
    }

public:
    // CRTP
    bool equal_to(const MyType& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }
    // CRTP
    VectorType difference(const MyType& other) const
    {
        return VectorType(elements[0] - other.x(), elements[1] - other.y());
    }
    // CRTP
    void translate(const VectorType& other)
    {
        elements[0] += other.x();
        elements[1] += other.y();
    }

public:
    T& x() {
        return elements[0];
    }
    const T& x() const {
        return elements[0];
    }
    T& y() {
        return elements[1];
    }
    const T& y() const {
        return elements[1];
    }

}; // struct Point2

} // namespace Discrete
} // namespace Math
} // namespace Ego
