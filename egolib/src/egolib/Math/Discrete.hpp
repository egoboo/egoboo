#pragma once

#include "egolib/platform.h"

#define Add(ResultType, OperandType0, OperandType1) \
    ResultType operator+=(const OperandType1& other) { \
        (*this) = (*this) + other; \
        return *this;\
    }
#define Subtract(ResultType, OperadnType0, OperandType1) \
    ResultType operator-=(const OperandType1& other) { \
        (*this) = (*this) - other; \
        return *this; \
    }
#define NotEqualTo(OperandType0,OperandType1) \
    bool operator!=(const OperandType1& other) const { \
        return !(*this == other); \
    }


namespace Ego {
namespace Math {
namespace Discrete {

template <typename T, typename Enabled = void>
struct Size2;

template <typename T>
struct Size2<T, std::enable_if_t<std::is_same<T, int>::value>> {
private:
    std::array<T, 2> elements;

public:
    Size2() : elements{0,0} {}
    Size2(T width, T height) : elements{width,height} {}
    Size2(const Size2<T>& other) : elements(other.elements) {}

public:
    const Size2<T>& operator=(const Size2<T>& other) {
        elements = other.elements;
        return *this;
    }
    bool operator==(const Size2<T>& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

public:
    // Derived operators
    NotEqualTo(Size2<T>, Size2<T>);

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
struct Vector2<T, std::enable_if_t<std::is_same<T, int>::value>> {
private:
    std::array<T, 2> elements;

public:
    Vector2() : elements{0,0} {}
    Vector2(T x, T y) : elements{x,y} {}
    Vector2(const Vector2<T>& other) : elements(other.elements) {}

public:
    const Vector2<T>& operator=(const Vector2<T>& other) {
        elements = other.elements;
        return *this;
    }
    Vector2<T> operator+(const Vector2<T>& other) const {
        return Vector2<T>(elements[0] + other.x(),
                          elements[1] + other.y());
    }
    Vector2<T> operator-(const Vector2<T>& other) const {
        return Vector2<T>(elements[0] - other.x(),
                          elements[1] - other.y());
    }
    bool operator==(const Vector2<T>& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

public:
    // Derived operators
    Add(Vector2<T>, Vector2<T>, Vector2<T>);
    Subtract(Vector2<T>, Vector2<T>, Vector2<T>);
    NotEqualTo(Vector2<T>, Vector2<T>);

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

}; // struct Vector2

template <typename T, typename Enabled = void>
struct Point2;

template <typename T>
struct Point2<T, std::enable_if_t<std::is_same<T, int>::value>> {
private:
    std::array<T, 2> elements;

public:
    Point2() : elements{0, 0} {}
    Point2(T x, T y) : elements{x,y} {}
    Point2(const Point2<T>& other) : elements(other.elements) {}

public:
    const Point2<T>& operator=(const Point2<T>& other) {
        elements = other.elements;
        return *this;
    }

public:
    bool operator==(const Point2<T>& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

public:
    Point2<T> operator+(const Vector2<T>& t) const {
        return Point2i(elements[0] + t.x(),
                       elements[1] + t.y());
    }
    Point2<T> operator-(const Vector2<T>& t) const {
        return Point2<T>(elements[0] - t.x(),
                         elements[1] - t.y());
    }
    Vector2<T> operator-(const Point2<T>& other) const {
        return Vector2<T>(elements[0] - other.x(),
                          elements[1] - other.y());
    }

public:
    // Derived operators
    Add(Point2<T>, Point2<T>, Vector2<T>);
    Subtract(Point2<T>, Point2<T>, Vector2<T>);
    NotEqualTo(Point2<T>, Point2<T>);

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

#undef Subtract
#undef NotEqualTo
#undef Add
