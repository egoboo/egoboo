//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file cartman_typedef.h
/// @details base type definitions and config options

#pragma once

#include "cartman/cartman_config.h"

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
        return Point2<T>(x - other.getX(), y - other.getY());
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

namespace Cartman {
typedef Ego::Math::Discrete::Vector2<int> Vector2i;
typedef Ego::Math::Discrete::Point2<int> Point2i;
typedef Ego::Math::Discrete::Size2<int> Size2i;
} // namespace Cartman

// Forward declarations.
struct camera_t;
struct cartman_mpd_t;
struct select_lst_t;
namespace Cartman { namespace Gui {
struct Cursor;
struct Manager;
struct Window;
} } // namespace Cartman::Gui

