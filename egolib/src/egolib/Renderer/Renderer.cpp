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

/// @file egolib/Renderer/Renderer.cpp
/// @brief Common interface of all renderers
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

StencilBuffer::StencilBuffer()
{}

StencilBuffer::~StencilBuffer()
{}

TextureUnit::TextureUnit()
{}

TextureUnit::~TextureUnit()
{}

Renderer::Renderer()
    : m_projectionMatrix(Math::Transform::perspective(Math::Degrees(45.0f), 4.0f/3.0f, +0.1f, +1.0f)),
      m_viewMatrix(Matrix4f4f::identity()), m_worldMatrix(Matrix4f4f::identity())
{}

Renderer::~Renderer()
{}

void Renderer::upload(egoboo_config_t& cfg)
{
    /* Nothing to do. */
}

void Renderer::download(egoboo_config_t& cfg)
{
    /* Nothing to do. */
}

void Renderer::setProjectionMatrix(const Matrix4f4f& projectionMatrix) {
    m_projectionMatrix = projectionMatrix;
}

Matrix4f4f Renderer::getProjectionMatrix() const {
    return m_projectionMatrix;
}

void Renderer::setViewMatrix(const Matrix4f4f& viewMatrix) {
    m_viewMatrix = viewMatrix;
}

Matrix4f4f Renderer::getViewMatrix() const {
    return m_viewMatrix;
}

void Renderer::setWorldMatrix(const Matrix4f4f& worldMatrix) {
    m_worldMatrix = worldMatrix;
}

Matrix4f4f Renderer::getWorldMatrix() const {
    return m_worldMatrix;
}

} // namespace Ego
