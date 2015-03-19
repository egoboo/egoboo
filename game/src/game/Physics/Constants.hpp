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

/// @file    game/Physics/Constants.hpp
/// @brief   invariants & defaults of the game's physics system

#pragma once

#include "egolib/egolib.h"

namespace Physics
{
    struct Constants
    {
#if 0
        /// @brief The gravity default amount and direction.
        /// @return the default gravity amount and direction
        /// @todo Should be a vector.
        static float gravity()
        {
            return -1.0f;
        }

        /// @brief The default friction of air.
        /// @return the default air friction
        /// @default 0.9868f
        static float airfriction()
        {
            return 0.9868f;
        }

        /// @brief The default friction for ice.
        /// @return the default ice friction
        /// @default 0.9738f (square of airfriction).
        static float icefriction()
        {
            return 0.9738f;
        }
#endif
    };
}
