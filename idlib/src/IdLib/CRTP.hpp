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

/// @file   IdLib/CRTP.hpp
/// @brief  CRTP (Curiously Recurring Template Pattern) for misc. operators.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Platform.hpp"

namespace Id {
/**
 * @brief
 *  Inherit from this class to define the postfix increment and prefix increment operators.
 *  The derived class needs to define a <tt>void increment();</tt> function.
 *  @code
 *  class Foo : Incrementable<Foo>
 *  {
 *  ...
 *  protected:
 *     void increment() override { ... }
 *  ...
 *  }
 *  @endcode
 * @author
 *  Michael Heilmann
 */
template <typename Derived>
class Incrementable {
public:
    Derived& operator++()
    { 
        static_cast<Derived *>(this)->increment();
        return *static_cast<Derived*>(this); 
    }
    Derived operator++(int)
    {
        auto it = *static_cast<Derived *>(this);
        ++(*static_cast<Derived *>(this));
        return *static_cast<Derived *>(&it);
    }
};

/**
 * @brief
 *	Inherit from this class to define the postfix decrement and prefix decremen operators.
 *  The derived class needs to define a <tt>void decrement();</tt> function.
 */
template <typename Derived>
class Decrementable {
public:
    Derived& operator--()
    { 
        static_cast<Derived *>(this)->decrement();
        return *static_cast<Derived *>(this); 
    }
    Derived operator--(int)
    {
        auto it = *static_cast<Derived *>(this);
        --(*static_cast<Derived *>(this));
        return *static_cast<Derived *>(&it);
    }
};

/**
 * @brief
 *	Inherit from this class to define the equality and inequality operators.
 *  The derived class needs to define a <tt>bool equalTo(const Derived&) const;</tt> function.
 */
template <typename Derived>
class Equatable {
public:
    bool operator==(const Derived& other) const
    {
        return static_cast<const Derived *>(this)->equalTo(other);
    }
    bool operator!=(const Derived& other) const
    {
        return !(*static_cast<const Derived *>(this) == other);
    }
};

/**
 * @brief
 *  Inherit from this class define the lower than, lower than or equal to,
 *  greater than, and greater than or equal to operators.
 *  The derived class needs to define a <tt>bool lowerThan(const Derived&) const;</tt> function.
 */
template <typename Derived>
class Comparable {
public:
	bool operator<(const Derived& other) const
    {
        return static_cast<const Derived *>(this)->lowerThan(other);
    }
    bool operator>(const Derived& other) const
    {
        return other < *static_cast<const Derived *>(this);
    }
    bool operator<=(const Derived& other) const
    {
        return !(*static_cast<const Derived *>(this) > other);
    }
    bool operator>=(const Derived& other) const
    {
        return !(*static_cast<const Derived *>(this) < other);
    }
};
 
} // namespace Id
