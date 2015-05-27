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

bool VALID_BILLBOARD_RANGE(BBOARD_REF ref) {
    return (ref >= 0)
        && (ref < BILLBOARDS_MAX);
}

bool VALID_BILLBOARD(BBOARD_REF ref) {
    return VALID_BILLBOARD_RANGE(ref)
        && BillboardSystem::get()._billboardList.get_ptr(ref)->_valid;
}

//--------------------------------------------------------------------------------------------
// billboard_data_t IMPLEMENTATION
//--------------------------------------------------------------------------------------------

billboard_data_t::billboard_data_t()
    : _valid(false), _endTime(0),
      _position(), _offset(), _offset_add(),
      _size(1), _size_add(0),
      _texture(nullptr), _obj_wptr(),
      _tint(Colour3f::white(), 1.0f), _tint_add(0.0f, 0.0f, 0.0f, 0.0f) {
    /* Intentionally empty. */
}

void billboard_data_t::set(bool valid, Uint32 endTime, std::shared_ptr<oglx_texture_t> texture)
{
    _valid = valid; _endTime = endTime;
    _position = fvec3_t::zero();
    _offset = fvec3_t::zero(); _offset_add = fvec3_t::zero();

    _texture = texture;
    _obj_wptr.reset();

    _tint = Colour4f(Colour3f::white(),1.0f);
    _tint_add = fvec4_t(0.0f, 0.0f, 0.0f, 0.0f);

    _size = 1.0f; _size_add = 0.0f;
}

void billboard_data_t::reset() {
    set(false, 0, nullptr);
}

void billboard_data_t::free() {
    if (!_valid) {
        return;
    }

    // Relinquish the texture.
    _texture = nullptr;

    reset();
}

bool billboard_data_t::update() {
    if (!_valid) {
        return false;
    }

    auto obj_ptr = _obj_wptr.lock();
    if (!obj_ptr || obj_ptr->isTerminated()) {
        return false;
    }

    // Determine where the new position should be.
    fvec3_t vup, pos_new;
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
        free();
    }

    return true;
}

//--------------------------------------------------------------------------------------------
// BillboardList IMPLEMENTATION
//--------------------------------------------------------------------------------------------

BillboardList::BillboardList() :
    lst() {
    clear_data();
}

void BillboardList::reset() {
    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref) {
        get_ptr(ref)->reset();
    }

    clear_data();
}

void BillboardList::update()
{
    Uint32 ticks = SDL_GetTicks();

    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        billboard_data_t *pbb = get_ptr(ref);

        if (!pbb->_valid) continue;

        bool isInvalid = false;
        if ((ticks >= pbb->_endTime) || (nullptr == pbb->_texture))
        {
            isInvalid = true;
        }
        auto obj_ref = pbb->_obj_wptr.lock();
        if (!obj_ref || obj_ref->isTerminated() || obj_ref->isBeingHeld() || obj_ref->isInsideInventory()) {
            isInvalid = true;
        }

        if (isInvalid) {
            // ... and deallocate it.
            free_one(ref);
        }
        else
        {
            get_ptr(ref)->update();
        }
    }
}

BBOARD_REF BillboardList::get_free_ref(Uint32 lifetime_secs, std::shared_ptr<oglx_texture_t> texture, const Ego::Math::Colour4f& tint, const BIT_FIELD opt_bits)
{
    if (free_count <= 0 || 0 == lifetime_secs) {
        return INVALID_BBOARD_REF;
    }

    BBOARD_REF ibb = INVALID_BBOARD_REF;
    size_t loops = 0;
    while (free_count > 0)
    {
        // grab the top index
        free_count--;
        update_guid++;

        ibb = free_ref[free_count];

        if (VALID_BILLBOARD_RANGE(ibb))
        {
            break;
        }

        loops++;
    }

    if (loops > 0)
    {
        log_warning("%s:%d: there is something wrong with the free stack. %" PRIuZ " loops.\n", __FILE__, __LINE__, loops);
    }

    if (!VALID_BILLBOARD_RANGE(ibb)) {
        return INVALID_BBOARD_REF;
    }

    billboard_data_t *pbb = get_ptr(ibb);

    pbb->set(true, SDL_GetTicks() + lifetime_secs * TICKS_PER_SEC, texture);
    pbb->_tint = tint;
    if (HAS_SOME_BITS(opt_bits, bb_opt_randomize_pos))
    {
        // make a random offset from the character
        pbb->_offset = fvec3_t(Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1)
            * (GRID_FSIZE / 5.0f);
    }

    if (HAS_SOME_BITS(opt_bits, bb_opt_randomize_vel))
    {
        // make the text fly away in a random direction
        pbb->_offset_add += fvec3_t(Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1, Random::nextFloat() * 2 - 1)
            * (2.0f * GRID_FSIZE / lifetime_secs / GameEngine::GAME_TARGET_UPS);
    }

    if (HAS_SOME_BITS(opt_bits, bb_opt_fade))
    {
        // make the billboard fade to transparency
        pbb->_tint_add[AA] = -1.0f / (lifetime_secs * GameEngine::GAME_TARGET_UPS);
    }

    if (HAS_SOME_BITS(opt_bits, bb_opt_burn))
    {
        float minval, maxval;

        minval = std::min({ pbb->_tint.getRed(), pbb->_tint.getGreen(), pbb->_tint.getBlue() });
        maxval = std::max({ pbb->_tint.getRed(), pbb->_tint.getGreen(), pbb->_tint.getBlue() });

        if (pbb->_tint.getRed() != maxval) {
            pbb->_tint_add[RR] = -pbb->_tint.getRed() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }

        if (pbb->_tint.getGreen() != maxval)
        {
            pbb->_tint_add[GG] = -pbb->_tint.getGreen() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }

        if (pbb->_tint.getBlue() != maxval)
        {
            pbb->_tint_add[BB] = -pbb->_tint.getBlue() / lifetime_secs / GameEngine::GAME_TARGET_UPS;
        }
    }

    return ibb;
}

bool BillboardList::free_one(BBOARD_REF ref)
{
    if (!VALID_BILLBOARD_RANGE(ref)) return false;
    billboard_data_t *pbb = get_ptr(ref);
    pbb->free();

#if defined(_DEBUG)
    {
        // Determine whether this billboard is already in the list of free billboards.
        // If this is the case, then this is an error.
        for (BBOARD_REF i = 0; i < free_count; ++i)
        {
            if (ref == free_ref[i]) return false;
        }
    }
#endif

    if (free_count >= BILLBOARDS_MAX)
    {
        return false;
    }

    free_ref[free_count] = ref;
    free_count++;
    update_guid++;

    return true;
}

void BillboardList::clear_data()
{
    /// @author BB
    /// @details reset the free billboard list.

    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        free_ref[ref] = REF_TO_INT(ref);
    }

    free_count = BILLBOARDS_MAX;
}

//--------------------------------------------------------------------------------------------

BillboardSystem *BillboardSystem::singleton = nullptr;

BillboardSystem::BillboardSystem() :
    _billboardList() {
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

bool BillboardSystem::render_one(billboard_data_t *pbb, float scale, const fvec3_t& cam_up, const fvec3_t& cam_rgt)
{
    if (NULL == pbb || !pbb->_valid) return false;

    auto obj_ptr = pbb->_obj_wptr.lock();
    // Do not display billboards for objects that are being held of are inside an inventory.
    if (!obj_ptr || obj_ptr->isTerminated() || obj_ptr->isBeingHeld() || obj_ptr->isInsideInventory()) {
        return false;
    }

    auto ptex = pbb->_texture.get();
    oglx_texture_t::bind(ptex);


    float s = (float)ptex->getSourceWidth() / (float)ptex->getWidth();
    float t = (float)ptex->getSourceHeight() / (float)ptex->getHeight();

    // @todo this billboard stuff needs to be implemented as a OpenGL transform

    // scale the camera vectors
    fvec3_t vec_rgt = cam_rgt * (ptex->getSourceWidth() * scale * pbb->_size);
    fvec3_t vec_up = cam_up  * (ptex->getSourceHeight() * scale * pbb->_size);

    GLvertex vtlist[4];
    // bottom left
    fvec3_t tmp;
    tmp = pbb->_position + (-vec_rgt - vec_up * 0);
    vtlist[0].pos[XX] = pbb->_offset[kX] + tmp[kX];
    vtlist[0].pos[YY] = pbb->_offset[kY] + tmp[kY];
    vtlist[0].pos[ZZ] = pbb->_offset[kZ] + tmp[kZ];
    vtlist[0].tex[SS] = s;
    vtlist[0].tex[TT] = t;

    // top left
    tmp = pbb->_position + (-vec_rgt + vec_up * 2);
    vtlist[1].pos[XX] = pbb->_offset[kX] + tmp[kX];
    vtlist[1].pos[YY] = pbb->_offset[kY] + tmp[kY];
    vtlist[1].pos[ZZ] = pbb->_offset[kZ] + tmp[kZ];
    vtlist[1].tex[SS] = s;
    vtlist[1].tex[TT] = 0;

    // top right
    tmp = pbb->_position + (vec_rgt + vec_up * 2);
    vtlist[2].pos[XX] = pbb->_offset[kX] + tmp[kX];
    vtlist[2].pos[YY] = pbb->_offset[kY] + tmp[kY];
    vtlist[2].pos[ZZ] = pbb->_offset[kZ] + tmp[kZ];
    vtlist[2].tex[SS] = 0;
    vtlist[2].tex[TT] = 0;

    // bottom right
    tmp = pbb->_position + (vec_rgt - vec_up * 0);
    vtlist[3].pos[XX] = pbb->_offset[kX] + tmp[kX];
    vtlist[3].pos[YY] = pbb->_offset[kY] + tmp[kY];
    vtlist[3].pos[ZZ] = pbb->_offset[kZ] + tmp[kZ];
    vtlist[3].tex[SS] = 0;
    vtlist[3].tex[TT] = t;

    {
        auto& renderer = Ego::Renderer::get();
        renderer.setColour(pbb->_tint);

        // Go on and draw it
        GL_DEBUG( glBegin )( GL_QUADS );
        {
            for (size_t i = 0; i < 4; ++i)
            {
                GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                GL_DEBUG( glVertex3fv )( vtlist[i].pos );
            }
        }
        GL_DEBUG_END();
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
            renderer.setGouraudShadingEnabled(false); // GL_LIGHTING_BIT

            // Draw only front-facing polygons.
            renderer.setCullingMode(Ego::CullingMode::Back);
            //GL_DEBUG(glEnable)(GL_CULL_FACE);  // GL_ENABLE_BIT 
            //glDisable(GL_LIGHTING);
            renderer.setBlendingEnabled(true);
            GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT

            // This drops 100% transparent fragments i.e. in order to pass, alpha has to be greater than 0.
            Ego::Renderer::get().setAlphaTestEnabled(true);
            GL_DEBUG(glAlphaFunc)(GL_GREATER, 0.0f);                          // GL_COLOR_BUFFER_BIT

            for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
            {
                billboard_data_t *pbb = _billboardList.get_ptr(ref);
                if (!pbb || !pbb->_valid ) continue;

                render_one(pbb, 0.75f, camera.getVUP(), camera.getVRT());
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();
}


