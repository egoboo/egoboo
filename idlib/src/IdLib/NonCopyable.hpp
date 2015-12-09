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

/// @file   IdLib/NonCopyable.hpp
/// @brief  Make classes non-copyable.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

namespace Id {
/**
 * @brief
 *  Inherit from this class to make the inheriting class and its descendant class non-copyable.
 *  Example usage
 *  @code
 *  class Foo : Bar, NonCopyable
 *  { ... }
 *  @endcode
 * @see http://en.cppreference.com/w/cpp/language/as_operator
 * @see http://en.cppreference.com/w/cpp/language/copy_constructor
 * @author
 *  Michael Heilmann
 */
class NonCopyable {

protected:
    NonCopyable() {}
    ~NonCopyable() {}
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace Id
