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

/// @file   egolib/Core/CollectionUtilities.hpp
/// @brief  Collection utility functions (list to set, ...)
/// @author Michael Heilmann
#pragma once

#include "egolib/platform.h"

namespace Ego
{

using namespace std;

/**
 * @brief
 *  Convert a vector to an unordered set.
 * @param v
 *  the vector
 * @return
 *  the unordered set
 * @remark
 *  This function is written in the spirit of std::make_shared_ptr, std::make_tuple, etc.
 */
template <typename Key, typename Hash = hash<Key>,
          typename Pred = equal_to<Key>,
          typename Alloc = allocator<Key> >
unordered_set<Key, Hash, Pred, Alloc> make_unordered_set(const vector<Key>& v)
{
    unordered_set<Key, Hash, Pred, Alloc> s;
    for (auto x : v)
    {
        s.insert(x);
    }
    return s;
}

} // namespace Ego
