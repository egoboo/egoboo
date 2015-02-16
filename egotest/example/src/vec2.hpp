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

// This is basically stolen from egolib/vec.h

#pragma once

#include <cmath>

enum {
    kX = 0,
    kY = 1,
};

struct fvec2_t
{
    
    union
    {
        float v[2];
        struct { float x, y; };
        struct { float s, t; };
    };
    
    const static fvec2_t zero;
    
    fvec2_t() : x(), y()
    {
    }
    
    fvec2_t(const fvec2_t& other) : x(other.x), y(other.y)
    {
    }
    
    fvec2_t(float x, float y)
    {
        this->x = x;
        this->y = y;
    }
    
    float dot(const fvec2_t& other) const
    {
        return this->v[kX] * other.v[kX]
        + this->v[kY] * other.v[kY]
        ;
    }
    
    void multiply(float scalar)
    {
        this->v[kX] *= scalar;
        this->v[kY] *= scalar;
    }
   
    void normalize(float length)
    {
        float l = this->length();
        if (l > 0.0f)
        {
            multiply(length / l);
        }
    }
    
    float normalize()
    {
        float l = length();
        if (l > 0.0f)
        {
            multiply(1.0f / l);
        }
        return l;
    }
    
    bool equals(const fvec2_t& other) const
    {
        return this->x == other.x
        && this->y == other.y;
    }
    
    float length_2() const
    {
        return this->v[kX] * this->v[kX]
        + this->v[kY] * this->v[kY]
        ;
    }
    
    float length() const
    {
        return std::sqrt(length_2());
    }
    
    float length_abs() const
    {
        return std::abs(this->v[kX]) + std::abs(this->v[kY]);
    }
    
    const fvec2_t& operator=(const fvec2_t& other)
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    
    fvec2_t operator+(const fvec2_t& other) const
    {
        return fvec2_t(x + other.x, y + other.y);
    }
    
    fvec2_t& operator+=(const fvec2_t& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    fvec2_t operator-(const fvec2_t& other) const
    {
        return fvec2_t(x - other.x, y - other.y);
    }
    
    fvec2_t& operator-=(const fvec2_t& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    fvec2_t operator*(const float other) const
    {
        return fvec2_t(other * v[kX], other * v[kY]);
    }
    
    fvec2_t& operator*=(float scalar)
    {
        multiply(scalar);
        return *this;
    }
    
    float& operator[](size_t const& index)
    {
#ifdef _DEBUG
        EGOBOO_ASSERT(index < 2);
#endif
        return this->v[index];
    }
    
    const float &operator[](size_t const& index) const
    {
#ifdef _DEBUG
        EGOBOO_ASSERT(index < 2);
#endif
        return this->v[index];
    }
    
};