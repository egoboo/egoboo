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

/// @file   egolib/Renderer/PrimitiveType.hpp
/// @brief  Primitive types.
/// @author Michael Heilmann

#pragma once

namespace Ego
{

/**
 * @brief
 *  An enumeration of primitive types.
 * @author
 *  Michael Heilmann
 */
enum class PrimitiveType
{
    /**
     * @brief
     *  Vertex \f$i\f$ defines point \f$i\f$.
     *  Given \f$n\f$ vertices, \f$n\f$ points are drawn.
     */
    Points,

    /**
     * @brief
     *  Vertices \f$2i - 1\f$ and \f$2i\f$ define line \f$i\f$.
     *  Given \f$n\f$ vertices, \f$\frac{n}{2}\f$ lines are drawn.
     */
    Lines,

    /**
     * @brief
     *  Vertices \f$3i-2\f$, \f$3i-1\f$, and \f$3i\f$ define triangle \f$i\f$.
     *  Given \f$n\f$ vertices, \f$\frac{n}{3}\f$ triangles are drawn.
     */
    Triangles,

    /**
     * @brief
     *  Vertices \f$4i-3\f$, \f$4i-2\f$, \f$4i-1\f$, and \f$4i\f$ define quadriliteral \f$i\f$.
     *  Given \f$n\f$ vertices, \f$\frac{n}{4}\f$ quadriliterals are drawn.
     */
    Quadriliterals,

    /**
     * @brief
     *  Vertices \f$1\f$, \$i+1\f$ and \f$i+2\f$ define triangle \f$i\f$.
     *  Given \f$n\f$ vertices, \f$n-2\f$ triangles are drawn.
     */
    TriangleFan,

    /**
     * @brief
     *  Vertices \f$i\f$,\f$i+1\f$, and \f$i+2\f$ define triangle \f$i\f$ if \f$i\f$ is odd.
     *  For even \f$i\f$, vertices \f$i+1\f$, \f$i\f$ , and \f$i+2\f$ define triangle \f$i\f$.
     *  Given \f$n\$ vertices, \f$n-2\f$ triangles are drawn.
     */
    TriangleStrip,
    
    /**
     * @brief
     *  Vertices \f$2i - 1\f$, \f$2i\f$, \f$2i + 2\f$, and \f$2i + 1\f$ define quadriliteral
     *  \f$i\f$. Given \f$n\f$ vertices, \f$\frac{n}{2}-1\f$ quadriliterals are drawn.
     */
    QuadriliteralStrip,

};

} // namespace Ego
