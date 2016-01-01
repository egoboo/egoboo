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

/// @file game/graphic_billboard.c
/// @brief
/// @details

#include "egolib/_math.h"
#include "game/graphic_billboard.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/Graphics/Camera.hpp"
#include "game/Entities/_Include.hpp"
#include "game/game.h"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/UIManager.hpp"
#include "game/CharacterMatrix.h"

Billboard::Billboard(Time::Ticks endTime, std::shared_ptr<Ego::Texture> texture, const float size)
    : _endTime(endTime),
      _position(), _offset(), _offset_add(),
      _size(size), _size_add(0.0f),
      _texture(texture), _object(),
      _tint(Colour3f::white(), 1.0f), _tint_add(0.0f, 0.0f, 0.0f, 0.0f) {
    /* Intentionally empty. */
}

bool Billboard::update(Time::Ticks now) {
    if ((now >= _endTime) || (nullptr == _texture)) {
        return false;
    }
    auto obj_ptr = _object.lock();
    if (!obj_ptr || obj_ptr->isTerminated() || obj_ptr->isBeingHeld() || obj_ptr->isInsideInventory()) {
        return false;
    }

    // Determine where the new position should be.
	Vector3f vup, pos_new;
    chr_getMatUp(obj_ptr.get(), vup);

    _size += _size_add;

    using namespace Ego::Math;
    _tint = Colour4f(constrain(_tint.getRed()   + _tint_add[kX], 0.0f, 1.0f),
                     constrain(_tint.getGreen() + _tint_add[kY], 0.0f, 1.0f),
                     constrain(_tint.getBlue()  + _tint_add[kZ], 0.0f, 1.0f),
                     constrain(_tint.getAlpha() + _tint_add[kW], 0.0f, 1.0f));

    /// @todo Why is this disabled. It should be there.
    //@note Zefz> because it looked bad in-game, only apply Z offset looks much better
    //_offset[kX] += _offset_add[kX];
    //_offset[kY] += _offset_add[kY];
    _offset.z() += _offset_add.z();

    // Automatically kill a billboard that is no longer useful.
    if (_tint.getAlpha() == 0.0f || _size <= 0.0f) {
        return false;
    }

    return true;
}

void BillboardSystem::update()
{
    const Time::Ticks ticks = Time::now<Time::Unit::Ticks>();

    _billboardList.erase(
        std::remove_if(_billboardList.begin(), _billboardList.end(), [&ticks](const std::shared_ptr<Billboard>& billboard) {
            return !billboard->update(ticks);
        }),
    _billboardList.end());
}

bool BillboardSystem::hasBillboard(const Object& object) const {
    for (const auto& billboard : _billboardList) {
        if (billboard->_object.lock().get() == &object) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Billboard> BillboardSystem::makeBillboard(Time::Seconds lifetime_secs, std::shared_ptr<Ego::Texture> texture, const Ego::Math::Colour4f& tint, const BIT_FIELD options, const float size) {
    auto billboard = std::make_shared<Billboard>(Time::now<Time::Unit::Ticks>() + lifetime_secs * TICKS_PER_SEC, texture, size);
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

        maxval = std::max({ billboard->_tint.getRed(), billboard->_tint.getGreen(), billboard->_tint.getBlue() });

        if (billboard->_tint.getRed() != maxval) {
            billboard->_tint_add[RR] = -billboard->_tint.getRed() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }

        if (billboard->_tint.getGreen() != maxval)
        {
            billboard->_tint_add[GG] = -billboard->_tint.getGreen() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }

        if (billboard->_tint.getBlue() != maxval)
        {
            billboard->_tint_add[BB] = -billboard->_tint.getBlue() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }
    }

    _billboardList.push_back(billboard);
    return billboard;
}

BillboardSystem *BillboardSystem::singleton = nullptr;

BillboardSystem::BillboardSystem() :
    _billboardList(), 
    vertexBuffer(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P3FT2F>()) {
}

BillboardSystem::~BillboardSystem() {
    reset();
}

void BillboardSystem::initialize() {
    if (!singleton) {
        singleton = new BillboardSystem();
    }
}

void BillboardSystem::uninitialize() {
    if (singleton) {
        delete singleton;
        singleton = nullptr;
    }
}

void BillboardSystem::reset() {
    _billboardList.clear();
}

BillboardSystem& BillboardSystem::get() {
    if (!singleton) {
        throw std::logic_error("billboard system is not initialized");
    }
    return *singleton;
}

bool BillboardSystem::render_one(Billboard& billboard, const Vector3f& cameraUp, const Vector3f& cameraRight)
{
    auto obj_ptr = billboard._object.lock();
    // Do not display billboards for objects that are being held of are inside an inventory.
    if (!obj_ptr || obj_ptr->isTerminated() || obj_ptr->isBeingHeld() || obj_ptr->isInsideInventory()) {
        return false;
    }

	auto texture = billboard._texture.get();

    // Compute the texture coordinates.
    float s = (float)texture->getSourceWidth()  / (float)texture->getWidth(),
          t = (float)texture->getSourceHeight() / (float)texture->getHeight();
    // Compute the scaled right and up vectors.
	Vector3f right = cameraRight * (texture->getSourceWidth()  * billboard._size),
             up    = cameraUp    * (texture->getSourceHeight() * billboard._size);

	{
        Vector3f tmp;
		Ego::VertexBufferScopedLock lock(vertexBuffer);
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
        auto& renderer = Ego::Renderer::get();
        renderer.setColour(billboard._tint);
		renderer.getTextureUnit().setActivated(texture);

        // Go on and draw it
		renderer.render(vertexBuffer, Ego::PrimitiveType::Quadriliterals, 0, 4);
    }

    return true;
}

void BillboardSystem::render_all(Camera& camera)
{
    gfx_begin_3d(camera);
    {
        Ego::OpenGL::PushAttrib pa(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        {
            auto& renderer = Ego::Renderer::get();
            // Do not write an incoming fragments depth value into the depth buffer.
            renderer.setDepthWriteEnabled(false);

            // Do not compare incoming fragments depth value with the depth buffer.
            renderer.setDepthTestEnabled(false);

            // flat shading
            renderer.setGouraudShadingEnabled(false);

            // Draw only front-facing polygons.
            renderer.setCullingMode(Ego::CullingMode::Back);

            //glDisable(GL_LIGHTING);
            renderer.setBlendingEnabled(true);
			renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

            // This drops 100% transparent fragments i.e. in order to pass, alpha has to be greater than 0.
            renderer.setAlphaTestEnabled(true);
			renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);

            for (const auto &billboard : _billboardList) {
                render_one(*billboard, camera.getUp(), camera.getRight());
            }
        }
    }
    gfx_end_3d();
}

std::shared_ptr<Billboard> BillboardSystem::makeBillboard(ObjectRef obj_ref, const std::string& text, const Ego::Math::Colour4f& textColor, const Ego::Math::Colour4f& tint, int lifetime_secs, const BIT_FIELD opt_bits, const float size)
{
    auto obj_ptr = _currentModule->getObjectHandler()[obj_ref];
    if (!obj_ptr) {
        return nullptr;
    }

    // Pre-render the text.
    std::shared_ptr<Ego::Texture> tex;
    try {
        tex = std::make_shared<Ego::OpenGL::Texture>();
    } catch (...) {
        return nullptr;
    }
    _gameEngine->getUIManager()->getFloatingTextFont()->drawTextToTexture(tex.get(), text, Ego::Math::Colour3f(textColor.getRed(), textColor.getGreen(), textColor.getBlue()));
    tex->setName("billboard text");

    // Create a new billboard.
    auto billboard = makeBillboard(lifetime_secs, tex, tint, opt_bits, size);
    if (!billboard) {
        return nullptr;
    }

    billboard->_object = std::weak_ptr<Object>(obj_ptr);
    billboard->_position = obj_ptr->getPosition();

    return billboard;
}
