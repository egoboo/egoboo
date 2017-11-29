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

/// @file egolib/Renderer/OpenGL/ColourBuffer.hpp
/// @brief Implementation of a colour buffer facade for OpenGL 2.1.
/// @author Michael Heilmann
#pragma once

#include "egolib/Renderer/Renderer.hpp"

namespace Ego {
namespace OpenGL {

using namespace Math;

class ColourBuffer : public Ego::ColourBuffer
{
private:
    id::rgba_depth colourDepth;

public:
    ColourBuffer();

    /// @brief Destruct this colour buffer facade.
    virtual ~ColourBuffer();

public:

    /** @copydoc Ego::Buffer<Colour4f>::clear */
    virtual void clear() override;

    /** @copydoc Ego::Buffer<Colour4f>::setClearValue */
    virtual void setClearValue(const Colour4f& value) override;

    /** @copydoc Ego::ColourBuffer::getColourDepth */
    virtual const id::rgba_depth& getColourDepth() override;

}; // class ColourBuffer

} // namespace OpenGL
} // namespace Ego
