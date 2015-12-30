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

/// @file   egolib/Renderer/Renderer.cpp
/// @brief  Common interface of all renderers
/// @author Michael Heilmann

#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Renderer/OpenGL/Renderer.hpp"

namespace Ego
{

namespace Core {

Renderer *CreateFunctor<Renderer>::operator()() const {
    return new Ego::OpenGL::Renderer();
}

}

AccumulationBuffer::AccumulationBuffer()
{}

AccumulationBuffer::~AccumulationBuffer()
{}

ColourBuffer::ColourBuffer()
{}

ColourBuffer::~ColourBuffer()
{}

DepthBuffer::DepthBuffer()
{}

DepthBuffer::~DepthBuffer()
{}

TextureUnit::TextureUnit()
{}

TextureUnit::~TextureUnit()
{}

Renderer::Renderer()
{}

Renderer::~Renderer()
{}

} // namespace Ego
