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

/// @file egolib/game/Graphics/BillboardSystem.cpp
/// @brief Billboard system

#include "egolib/game/Graphics/BillboardSystem.hpp"
#include "egolib/game/Graphics/Billboard.hpp"
#include "egolib/game/renderer_3d.h"
#include "egolib/game/Graphics/Camera.hpp"
#include "egolib/Entities/_Include.hpp"
#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/game/GUI/UIManager.hpp"
#include "egolib/game/CharacterMatrix.h"
#include "egolib/Graphics/VertexFormat.hpp"

namespace Ego {
namespace Graphics {

void BillboardSystem::update()
{
    const ::Time::Ticks ticks = ::Time::now<::Time::Unit::Ticks>();

    _billboardList.erase(
        std::remove_if(_billboardList.begin(), _billboardList.end(), [&ticks](const std::shared_ptr<Ego::Graphics::Billboard>& billboard)
    {
        return !billboard->update(ticks);
    }),
        _billboardList.end());
}

bool BillboardSystem::hasBillboard(const Object& object) const
{
    for (const auto& billboard : _billboardList)
    {
        if (billboard->_object.lock().get() == &object)
        {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Billboard> BillboardSystem::makeBillboard(::Time::Seconds lifetime_secs, std::shared_ptr<Ego::Texture> texture, const Ego::Colour4f& tint, const BIT_FIELD options, const float size)
{
    auto billboard = std::make_shared<Billboard>(::Time::now<::Time::Unit::Ticks>() + lifetime_secs * TICKS_PER_SEC, texture, size);
    billboard->_tint = tint;

    if (HAS_SOME_BITS(options, Billboard::Flags::RandomPosition))
    {
        // make a random offset from the character
        billboard->_offset = Vector3f(Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1)
            * (Info<float>::Grid::Size() / 5.0f);
    }

    if (HAS_SOME_BITS(options, Billboard::Flags::RandomVelocity))
    {
        // make the text fly away in a random direction
        billboard->_offset_add += Vector3f(Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1)
            * (2.0f * Info<float>::Grid::Size() / lifetime_secs / GameEngine::GAME_TARGET_UPS);
    }

    if (HAS_SOME_BITS(options, Billboard::Flags::Fade))
    {
        // make the billboard fade to transparency
        billboard->_tint_add[AA] = -1.0f / (lifetime_secs * GameEngine::GAME_TARGET_UPS);
    }

    if (HAS_SOME_BITS(options, Billboard::Flags::Burn))
    {
        float maxval;

        maxval = std::max({billboard->_tint.get_r(), billboard->_tint.get_g(), billboard->_tint.get_b()});

        if (billboard->_tint.get_r() != maxval)
        {
            billboard->_tint_add[RR] = -billboard->_tint.get_r() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }

        if (billboard->_tint.get_g() != maxval)
        {
            billboard->_tint_add[GG] = -billboard->_tint.get_g() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }

        if (billboard->_tint.get_b() != maxval)
        {
            billboard->_tint_add[BB] = -billboard->_tint.get_b() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }
    }

    _billboardList.push_back(billboard);
    return billboard;
}

BillboardSystem::BillboardSystem() :
    _billboardList(),
    vertexDescriptor(Ego::descriptor_factory<idlib::vertex_format::P3FT2F>()()),
    vertexBuffer(idlib::video_buffer_manager::get().create_vertex_buffer(4, vertexDescriptor.get_size()))
{}

BillboardSystem::~BillboardSystem()
{
    reset();
}

void BillboardSystem::reset()
{
    _billboardList.clear();
}

bool BillboardSystem::render_one(Billboard& billboard, const Vector3f& cameraUp, const Vector3f& cameraRight)
{
    auto obj_ptr = billboard._object.lock();
    // Do not display billboards for objects that are being held of are inside an inventory.
    if (!obj_ptr || obj_ptr->isTerminated() || obj_ptr->isBeingHeld() || obj_ptr->isInsideInventory())
    {
        return false;
    }

    auto texture = billboard._texture.get();

    // Compute the texture coordinates.
    float s = (float)texture->getSourceWidth() / (float)texture->getWidth(),
        t = (float)texture->getSourceHeight() / (float)texture->getHeight();
    // Compute the scaled right and up vectors.
    Vector3f right = cameraRight * (texture->getSourceWidth()  * billboard._size),
        up = cameraUp    * (texture->getSourceHeight() * billboard._size);

    {
        Vector3f tmp;
        idlib::vertex_buffer_scoped_lock lock(*vertexBuffer);
        Vertex *vertices = lock.get<Vertex>();

        // bottom left
        tmp = billboard._position + billboard._offset + (-right - up * 0);
        vertices[0].x = tmp.x();
        vertices[0].y = tmp.y();
        vertices[0].z = tmp.z();
        vertices[0].s = s;
        vertices[0].t = t;

        // top left
        tmp = billboard._position + billboard._offset + (-right + up * 2);
        vertices[1].x = tmp.x();
        vertices[1].y = tmp.y();
        vertices[1].z = tmp.z();
        vertices[1].s = s;
        vertices[1].t = 0;

        // top right
        tmp = billboard._position + billboard._offset + (right + up * 2);
        vertices[2].x = tmp.x();
        vertices[2].y = tmp.y();
        vertices[2].z = tmp.z();
        vertices[2].s = 0;
        vertices[2].t = 0;

        // bottom right
        tmp = billboard._position + billboard._offset + (right - up * 0);
        vertices[3].x = tmp.x();
        vertices[3].y = tmp.y();
        vertices[3].z = tmp.z();
        vertices[3].s = 0;
        vertices[3].t = t;
    }

    {
        auto& renderer = Renderer::get();
        renderer.setColour(billboard._tint);
        renderer.getTextureUnit().setActivated(texture);

        // Go on and draw it
        renderer.render(*vertexBuffer, vertexDescriptor, idlib::primitive_type::quadriliterals, 0, 4);
    }

    return true;
}

void BillboardSystem::render_all(::Camera& camera)
{
    Renderer3D::begin3D(camera);
    {
        OpenGL::PushAttrib pa(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        {
            auto& renderer = Renderer::get();
            // Do not write an incoming fragments depth value into the depth buffer.
            renderer.setDepthWriteEnabled(false);

            // Do not compare incoming fragments depth value with the depth buffer.
            renderer.setDepthTestEnabled(false);

            // flat shading
            renderer.setGouraudShadingEnabled(false);

            // Draw only front-facing polygons.
            renderer.setCullingMode(idlib::culling_mode::back);

            renderer.setBlendingEnabled(true);
            renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);

            // This drops 100% transparent fragments i.e. in order to pass, alpha has to be greater than 0.
            renderer.setAlphaTestEnabled(true);
            renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);

            for (const auto &billboard : _billboardList)
            {
                render_one(*billboard, camera.getUp(), camera.getRight());
            }
        }
    }
    Renderer3D::end3D();
}

std::shared_ptr<Billboard> BillboardSystem::makeBillboard(ObjectRef obj_ref, const std::string& text, const Ego::Colour4f& textColor, const Ego::Colour4f& tint, int lifetime_secs, const BIT_FIELD opt_bits, const float size)
{
    auto obj_ptr = _currentModule->getObjectHandler()[obj_ref];
    if (!obj_ptr)
    {
        return nullptr;
    }

    // Pre-render the text.
    std::shared_ptr<Texture> tex;
    try
    {
        tex = Ego::Renderer::get().createTexture();
    }
    catch (...)
    {
        return nullptr;
    }
    _gameEngine->getUIManager()->getFloatingTextFont()->drawTextToTexture(tex.get(), text, Ego::Colour3f(textColor.get_r(), textColor.get_g(), textColor.get_b()));
    tex->setName("billboard text");

    // Create a new billboard.
    auto billboard = makeBillboard(lifetime_secs, tex, tint, opt_bits, size);
    if (!billboard)
    {
        return nullptr;
    }

    billboard->_object = std::weak_ptr<Object>(obj_ptr);
    billboard->_position = obj_ptr->getPosition();

    return billboard;
}

} // namespace Graphics
} // namespace Ego
