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
    int width, height;

public:
    Size2() : width(), height() {}
    Size2(T width, T height) : width(width), height(height) {}
    Size2(const Size2<T>& other) : width(other.width), height(other.height) {}

public:
    const Size2<T>& operator=(const Size2<T>& other) {
        width = other.width;
        height = other.height;
        return *this;
    }
    bool operator==(const Size2<T>& other) const {
        return width == other.width
            && height == other.height;
    }

public:
    // Derived operators
    NotEqualTo(Size2<T>, Size2<T>);

public:
    T getWidth() const {
        return width;
    }
    T getHeight() const {
        return height;
    }

}; // struct Size2

template <typename T, typename Enabled = void>
struct Vector2;

template <typename T>
struct Vector2<T, std::enable_if_t<std::is_same<T, int>::value>> {
private:
    T x, y;

public:
    Vector2() : x(), y() {}
    Vector2(T x, T y) : x(x), y(y) {}
    Vector2(const Vector2<T>& other) : x(other.x), y(other.y) {}

public:
    const Vector2<T>& operator=(const Vector2<T>& other) {
        x = other.x;
        y = other.y;
        return *this;
    }
    Vector2<T> operator+(const Vector2<T>& other) const {
        return Vector2<T>(x + other.getX(), y + other.getY());
    }
    Vector2<T> operator-(const Vector2<T>& other) const {
        return Vector2<T>(x - other.getX(), y - other.getY());
    }
    bool operator==(const Vector2<T>& other) const {
        return x == other.x
            && y == other.y;
    }

public:
    // Derived operators
    Add(Vector2<T>, Vector2<T>, Vector2<T>);
    Subtract(Vector2<T>, Vector2<T>, Vector2<T>);
    NotEqualTo(Vector2<T>, Vector2<T>);

public:
    int getX() const {
        return x;
    }
    int getY() const {
        return y;
    }

}; // struct Vector2

template <typename T, typename Enabled = void>
struct Point2;

template <typename T>
struct Point2<T, std::enable_if_t<std::is_same<T, int>::value>> {
private:
    T x, y;

public:
    Point2() : x(0), y(0) {}
    Point2(T x, T y) : x(x), y(y) {}
    Point2(const Point2<T>& other) : x(other.x), y(other.y) {}

public:
    const Point2<T>& operator=(const Point2<T>& other) {
        x = other.x;
        y = other.y;
        return *this;
    }

public:
    bool operator==(const Point2<T>& other) const {
        return x == other.x
            && y == other.y;
    }

public:
    Point2<T> operator+(const Vector2<T>& t) const {
        return Point2i(x + t.getX(), y + t.getY());
    }
    Point2<T> operator-(const Vector2<T>& t) const {
        return Point2<T>(x - t.getX(), y - t.getY());
    }
    Vector2<T> operator-(const Point2<T>& other) const {
        return Vector2<T>(x - other.getX(), y - other.getY());
    }

public:
    // Derived operators
    Add(Point2<T>, Point2<T>, Vector2<T>);
    Subtract(Point2<T>, Point2<T>, Vector2<T>);
    NotEqualTo(Point2<T>, Point2<T>);

public:
    int getX() const {
        return x;
    }
    int getY() const {
        return y;
    }

}; // struct Point2

} // namespace Discrete
} // namespace Math
} // namespace Ego

#undef Subtract
#undef NotEqualTo
#undef Add
