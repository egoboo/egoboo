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

/// @file egolib/Renderer/OpenGL/AccumulationBuffer.hpp
/// @brief Implementation of an accumulation buffer facade for OpenGL 2.1.
/// @author Michael Heilmann

#pragma once

#include "egolib/Renderer/Renderer.hpp"

namespace Ego {
namespace OpenGL {

using namespace Math;

class AccumulationBuffer : public Ego::AccumulationBuffer
{
private:
    idlib::rgba_depth colourDepth;

public:
    /// @brief Construct this accumulation buffer facade.
    AccumulationBuffer();

    /// @brief Destruct this accumulation buffer facade.
    virtual ~AccumulationBuffer();

public:
    /** @copydoc Ego::Buffer<Ego::Colour4f>::clear */
    virtual void clear() override;

    /** @copydoc Ego::Buffer<Ego::Colour4f>::setClearValue */
    virtual void setClearValue(const Colour4f& value) override;

    /** @copydoc Ego::AccumulationBuffer::getColourDepth */
    virtual const idlib::rgba_depth& getColourDepth() override;

}; // class AccumulationBuffer

} // namespace OpenGL
} // namespace Ego
