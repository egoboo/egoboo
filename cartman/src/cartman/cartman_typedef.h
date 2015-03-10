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

struct Vector2i
{
private:
    int _x, _y;
public:
    Vector2i(int x,int y) :
        _x(x), _y(y)
    {}
    Vector2i(const Vector2i& other) :
        _x(other._x), _y(other._y)
    {}
    int operator()(size_t i) const
    {
        switch (i)
        {
        case 0:
            return _x;
        case 1:
            return _y;
        default:
            throw std::invalid_argument("i > 1");
        }
    }
    int& operator()(size_t i)
    {
        switch (i)
        {
        case 0:
            return _x;
        case 1:
            return _y;
        default:
            throw std::invalid_argument("i > 1");
        }
    }
};

struct Point2i
{
private:
    int _x, _y;
public:
    Point2i(int x, int y) :
        _x(x), _y(y)
    {}
    Point2i(const Point2i& other) :
        _x(other._x), _y(other._y)
    {}
    void translate(const Vector2i& t)
    {
        _x += t(0);
        _y += t(1);
    }
    int operator()(size_t i)
    {
        switch (i)
        {
        case 0:
            return _x;
        case 1:
            return _y;
        default:
            throw std::invalid_argument("i > 1");
        } 
    }
};
