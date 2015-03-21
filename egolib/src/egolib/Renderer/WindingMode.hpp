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

/// @file   egolib/Renderer/WindingMode.hpp
/// @brief  An enumeration of winding modes.
/// @author Michael Heilmann

namespace Ego
{
    /**
     * @brief
     *  An enumeration of winding modes.
     *  
     *  The projection of a polygon to window coordinates is said to have clockwise winding if an
     *  imaginary object following the path from its first vertex, its second vertex, and so on, to its
     *  last vertex, and finally back to its first vertex, moves in a clockwise direction about the
     *  interior of the polygon. The polygon's winding is said to be anticlockwise if the imaginary
     *  object following the same path moves in a anticlockwise direction about the interior of the
     *  polygon.
     *
     *  The winding mode specifies whether polygons with clockwise winding in window coordinates, or
     *  anticlockwise winding in window coordinates, are taken to be front-facing. Certain operations -
     *  in particular but not restricted to culling and tesselation - are influenced by the polygon
     *  winding.
     */
    enum class WindingMode
    {
        /**
         * @brief
         *  Clockwise polygons are front-facing.
         */
         Clockwise,

        /**
         * @brief
         *  Anticlockwise polygons are front-facing.
         */
        AntiClockwise,
    };
}
