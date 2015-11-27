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

/// @file   egolib/Renderer/CullingMode.hpp
/// @brief  Enumeration of culling modes.
/// @author Michael Heilmann

#pragma once

namespace Ego {

/**
 * @brief
 *  An enumeration of culling modes.
 */
enum class CullingMode {

    /**
     * @brief
     *  Neither front-facing nor back-facing polygons are culled.
     */
    None,

    /**
     * @brief
     *  Front-facing polygons are culled.
     */
    Front,

    /**
     * @brief
     *  Back-facing polygons are culled.
     */
    Back,

    /**
     * @brief
     *  Back-facing and front-facing polygons are culled.
     */
    BackAndFront,
    
    _COUNT, ///< @todo Remove this.

}; // enum class CullingMode

} // namespace Ego
