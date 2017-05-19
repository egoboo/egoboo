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

/// @file egolib/Renderer/OpenGL/TextureUnit.hpp
/// @brief Implementation of a texture unit facade for OpenGL 2.1. 
/// @author Michael Heilmann
#pragma once

#include "egolib/Renderer/Renderer.hpp"

namespace Ego {
namespace OpenGL {

class Texture;
class RendererInfo;

class TextureUnit : public Ego::TextureUnit
{
private:
    std::shared_ptr<RendererInfo> m_info;
public:
    /// @brief Construct this texture unit facade.
    /// @param info pointer to the render device information
    TextureUnit(const std::shared_ptr<RendererInfo>& info);

    /// @brief Destruct this texture unit facade.
    virtual ~TextureUnit();

    /** @copydoc Ego::TextureUnit::setActivated */
    void setActivated(const Ego::Texture *texture) override;

    void setActivated(Texture *texture);

}; // class TextureUnit

} // namespace OpenGL
} // namespace Ego
