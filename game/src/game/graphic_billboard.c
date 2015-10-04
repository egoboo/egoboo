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
#include "game/char.h"
#include "game/Entities/_Include.hpp"
#include "game/game.h"
#include "game/Core/GameEngine.hpp"

Billboard::Billboard(Uint32 endTime, std::shared_ptr<oglx_texture_t> texture)
    : _endTime(endTime),
      _position(), _offset(), _offset_add(),
      _size(1), _size_add(0),
      _texture(texture), _obj_wptr(),
      _tint(Colour3f::white(), 1.0f), _tint_add(0.0f, 0.0f, 0.0f, 0.0f) {
    /* Intentionally empty. */
}

bool Billboard::update(Uint32 ticks) {
    if ((ticks >= _endTime) || (nullptr == _texture)) {
        return false;
    }
    auto obj_ptr = _obj_wptr.lock();
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
    //_offset[kX] += _offset_add[kX];
    //_offset[kY] += _offset_add[kY];
    _offset[kZ] += _offset_add[kZ];

    // Automatically kill a billboard that is no longer useful.
    if (_tint.getAlpha() == 0.0f || _size <= 0.0f) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
// BillboardList IMPLEMENTATION
//--------------------------------------------------------------------------------------------

BillboardList::BillboardList() :
    _used() {
}

void BillboardList::reset() {
    update(std::numeric_limits<Uint32>::max());
}

void BillboardList::update(Uint32 ticks)
{
    auto i = _used.begin(),
        e = _used.end();

    while (i != e) {
        if (!(*i)->update(ticks)) {
            i = _used.erase(i);
        }
        else {
            i++;
        }
    }
}

void BillboardList::update()
{
    update(SDL_GetTicks());
}

std::shared_ptr<Billboard> BillboardList::makeBillboard(Uint32 lifetime_secs, std::shared_ptr<oglx_texture_t> texture, const Ego::Math::Colour4f& tint, const BIT_FIELD options) {
    auto billboard = std::make_shared<Billboard>(SDL_GetTicks() + lifetime_secs * TICKS_PER_SEC, texture);
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
    _used.push_back(billboard);
    return billboard;
}

//--------------------------------------------------------------------------------------------

BillboardSystem *BillboardSystem::singleton = nullptr;

BillboardSystem::BillboardSystem() :
    _billboardList(), _vertexBuffer(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P3FT2F>()) {
}

BillboardSystem::~BillboardSystem() {
    _billboardList.reset();
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
    _billboardList.reset();
}

BillboardSystem& BillboardSystem::get() {
    if (!singleton) {
        throw std::logic_error("billboard system is not initialized");
    }
    return *singleton;
}

bool BillboardSystem::render_one(Billboard& bb, float scale, const Vector3f& cam_up, const Vector3f& cam_rgt)
{
    auto obj_ptr = bb._obj_wptr.lock();
    // Do not display billboards for objects that are being held of are inside an inventory.
    if (!obj_ptr || obj_ptr->isTerminated() || obj_ptr->isBeingHeld() || obj_ptr->isInsideInventory()) {
        return false;
    }

	auto ptex = bb._texture.get();

    float s = (float)ptex->getSourceWidth() / (float)ptex->getWidth();
    float t = (float)ptex->getSourceHeight() / (float)ptex->getHeight();

    // @todo this billboard stuff needs to be implemented as a OpenGL transform

    // scale the camera vectors
	Vector3f vec_rgt = cam_rgt * (ptex->getSourceWidth() * scale * bb._size);
	Vector3f vec_up = cam_up  * (ptex->getSourceHeight() * scale * bb._size);

    // bottom left
	Vector3f tmp;
	{
		Ego::VertexBufferScopedLock lock(_vertexBuffer);
		Vertex *vertices = lock.get<Vertex>();
		tmp = bb._position + (-vec_rgt - vec_up * 0);
		vertices[0].x = bb._offset[kX] + tmp[kX];
		vertices[0].y = bb._offset[kY] + tmp[kY];
		vertices[0].z = bb._offset[kZ] + tmp[kZ];
		vertices[0].s = s;
		vertices[0].t = t;

		// top left
		tmp = bb._position + (-vec_rgt + vec_up * 2);
		vertices[1].x = bb._offset[kX] + tmp[kX];
		vertices[1].y = bb._offset[kY] + tmp[kY];
		vertices[1].z = bb._offset[kZ] + tmp[kZ];
		vertices[1].s = s;
		vertices[1].t = 0;

		// top right
		tmp = bb._position + (vec_rgt + vec_up * 2);
		vertices[2].x = bb._offset[kX] + tmp[kX];
		vertices[2].y = bb._offset[kY] + tmp[kY];
		vertices[2].z = bb._offset[kZ] + tmp[kZ];
		vertices[2].s = 0;
		vertices[2].t = 0;

		// bottom right
		tmp = bb._position + (vec_rgt - vec_up * 0);
		vertices[3].x = bb._offset[kX] + tmp[kX];
		vertices[3].y = bb._offset[kY] + tmp[kY];
		vertices[3].z = bb._offset[kZ] + tmp[kZ];
		vertices[3].s = 0;
		vertices[3].t = t;
	}

    {
        auto& renderer = Ego::Renderer::get();
        renderer.setColour(bb._tint);
		renderer.getTextureUnit().setActivated(ptex);

        // Go on and draw it
		renderer.render(_vertexBuffer, Ego::PrimitiveType::Quadriliterals, 0, 4);
    }

    return true;
}

void BillboardSystem::render_all(Camera& camera)
{
    gfx_begin_3d(camera);
    {
        ATTRIB_PUSH( __FUNCTION__, GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
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

            for (auto bb : _billboardList._used) {
                render_one(*bb, 0.75f, camera.getUp(), camera.getRight());
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();
}


