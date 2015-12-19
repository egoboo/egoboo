#pragma once

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

#include "egolib/egolib.h"

#include "cartman/cartman_config.h"

namespace Cartman {

struct Size2i {
    int width, height;
public:
    Size2i(int width, int height) : width(width), height(height) {}
    Size2i(const Size2i& other) : width(other.width), height(other.height) {}
public:
    const Size2i& operator=(const Size2i& other) {
        width = other.width;
        height = other.height;
        return *this;
    }
public:
    bool operator==(const Size2i& other) const {
        return width == other.width
            && height == other.height;
    }
    bool operator!=(const Size2i& other) const {
        return width != other.width
            || height != other.height;
    }
};

struct Vector2i {
private:
    int x, y;
public:
    Vector2i() :
        x(0), y(0) {}
    Vector2i(int x, int y) :
        x(x), y(y) {}
    Vector2i(const Vector2i& other) :
        x(other.x), y(other.y) {}
    const int& operator()(size_t i) const {
        switch (i) {
            case 0:
                return x;
            case 1:
                return y;
            default:
                throw std::invalid_argument("i > 1");
        }
    }
    int& operator()(size_t i) {
        switch (i) {
            case 0:
                return x;
            case 1:
                return y;
            default:
                throw std::invalid_argument("i > 1");
        }
    }
};
}

namespace Cartman {
struct Point2i {
private:
    int x, y;
public:
    Point2i() :
        x(0), y(0) {}
    Point2i(int x, int y) :
        x(x), y(y) {}
    Point2i(const Point2i& other) :
        x(other.x), y(other.y) {}
public:
    Point2i& operator=(const Point2i& other) {
        x = other.x;
        y = other.y;
        return *this;
    }
public:
    bool operator==(const Point2i& other) const {
        return x == other.x
            && y == other.y;
    }
    bool operator!=(const Point2i& other) const {
        return x != other.x
            || y != other.y;
    }
public:
    void translate(const Vector2i& t) {
        x += t(0);
        y += t(1);
    }
public:
    const int& operator()(size_t i) const {
        switch (i) {
            case 0:
                return x;
            case 1:
                return y;
            default:
                throw std::invalid_argument("i > 1");
        }
    }
    int& operator()(size_t i) {
        switch (i) {
            case 0:
                return x;
            case 1:
                return y;
            default:
                throw std::invalid_argument("i > 1");
        }
    }
};
}
