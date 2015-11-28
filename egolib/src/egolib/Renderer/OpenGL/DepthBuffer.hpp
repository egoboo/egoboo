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

/// @file   egolib/Renderer/OpenGL/DepthBuffer.hpp
/// @brief  Implementation of a depth buffer facade for OpenGL 2.1.
/// @author Michael Heilmann
#pragma once

#include "egolib/Renderer/Renderer.hpp"

namespace Ego
{
namespace OpenGL
{

class DepthBuffer : public Ego::DepthBuffer
{
public:
    
    /**
     * @brief
     *  Construct this depth buffer facade.
     */
    DepthBuffer();
    
    /**
     * @brief
     *  Destruct this depth buffer facade.
     */
    virtual ~DepthBuffer();

public:

    /** @copydoc Ego::Buffer<float>::clear */
    virtual void clear() override;

    /** @copydoc Ego::Buffer<float>::setClearValue */
    virtual void setClearValue(const float& value) override;

};

} // namespace OpenGL
} // namespace Ego
