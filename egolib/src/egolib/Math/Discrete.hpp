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
    typedef Size2<T> MyType;
    std::array<T, 2> elements;

public:
    Size2() : elements{0,0} {}
    Size2(T width, T height) : elements{width,height} {}
    Size2(const MyType& other) : elements(other.elements) {}

public:
    const MyType& operator=(const MyType& other) {
        elements = other.elements;
        return *this;
    }
    bool operator==(const MyType& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

public:
    // Derived operators
    NotEqualTo(MyType, MyType);

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
    typedef Vector2<T> MyType;
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
    MyType operator+(const MyType& other) const {
        return MyType(elements[0] + other.elements[0],
                      elements[1] + other.elements[1]);
    }
    MyType operator-(const MyType& other) const {
        return MyType(elements[0] - other.elements[0],
                      elements[1] - other.elements[1]);
    }
    bool operator==(const MyType& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

public:
    // Derived operators
    Add(MyType, MyType, MyType);
    Subtract(MyType, MyType, MyType);
    NotEqualTo(MyType, MyType);

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
    typedef Vector2<T> VectorType;
    typedef Point2<T> MyType;
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
    bool operator==(const MyType& other) const {
        return elements[0] == other.elements[0]
            && elements[1] == other.elements[1];
    }

public:
    MyType operator+(const VectorType& t) const {
        return MyType(elements[0] + t.x(), elements[1] + t.y());
    }
    MyType operator-(const VectorType& t) const {
        return MyType(elements[0] - t.x(), elements[1] - t.y());
    }
    VectorType operator-(const MyType& other) const {
        return VectorType(elements[0] - other.x(), elements[1] - other.y());
    }

public:
    // Derived operators
    Add(MyType, MyType, VectorType);
    Subtract(MyType, MyType, VectorType);
    NotEqualTo(MyType, MyType);

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
