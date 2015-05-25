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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

BillboardList g_billboardList;

static bool _billboard_system_started = false;

//--------------------------------------------------------------------------------------------
// billboard_data_t IMPLEMENTATION
//--------------------------------------------------------------------------------------------

billboard_data_t::billboard_data_t()
    : _valid(false), _endTime(0),
      _position(), _offset(), _offset_add(),
      _size(1), _size_add(0),
      _tex_ref(INVALID_TX_REF), _obj_ref(INVALID_CHR_REF),
      _tint(Colour3f::white(), 1.0f), _tint_add(0.0f, 0.0f, 0.0f, 0.0f) {
    /* Intentionally empty. */
}

billboard_data_t *billboard_data_t::init(bool valid, Uint32 endTime, TX_REF tex_ref)
{
    _valid = valid; _endTime = endTime;
    _position = fvec3_t::zero();
    _offset = fvec3_t::zero(); _offset_add = fvec3_t::zero();

    _tex_ref = tex_ref;
    _obj_ref = INVALID_CHR_REF;

    _tint = Colour4f(Colour3f::white(),1.0f);
    _tint_add = fvec4_t(0.0f, 0.0f, 0.0f, 0.0f);

    _size = 1.0f; _size_add = 0.0f;

    return this;
}

billboard_data_t *billboard_data_t::init()
{
    return init(false, 0, INVALID_TX_REF);
}

bool billboard_data_t::free() {
    if (!_valid) {
        return false;
    }

    // Relinquish the texture.
    TextureManager::get().relinquish(_tex_ref);

    init();

    return true;
}

bool billboard_data_t::update() {
    if (!_valid) {
        return false;
    }

    if (!_gameObjects.exists(_obj_ref)) {
        return false;
    }
    
    Object *obj_ptr = _gameObjects.get(_obj_ref);

    // Determine where the new position should be.
    fvec3_t vup, pos_new;
    chr_getMatUp(obj_ptr, vup);

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
    if (_tint.getAlpha() == 0.0f || _size <= 0.0f)
    {
        free();
    }

    return true;
}

bool billboard_data_t::printf_ttf(const std::shared_ptr<Ego::Font> &font, const Ego::Math::Colour4f& color, const char * format, ...)
{
    if (!_valid) {
        return false;
    }

    Ego::Math::Colour3f fontColour = Ego::Math::Colour3f(color.getRed(), color.getGreen(), color.getBlue());

    // release any existing texture in case there is an error
    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(_tex_ref);
    ptex->release();

    va_list args;
    char buffer[256];
    va_start(args, format);
    vsnprintf(buffer, SDL_arraysize(buffer), format, args);
    va_end(args);
    
    font->drawTextToTexture(ptex, buffer, fontColour);

    ptex->setName("billboard text");

    return true;
}

//--------------------------------------------------------------------------------------------
// BillboardList IMPLEMENTATION
//--------------------------------------------------------------------------------------------


void BillboardList::init_all()
{
    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        get_ptr(ref)->init();
    }

    clear_data();
}

void BillboardList::update_all()
{
    Uint32 ticks = SDL_GetTicks();

    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        billboard_data_t *pbb = get_ptr(ref);

        if (!pbb->_valid) continue;

        bool is_invalid = false;
        if ((ticks >= pbb->_endTime) || (nullptr == TextureManager::get().get_valid_ptr(pbb->_tex_ref)))
        {
            is_invalid = true;
        }

        if (!_gameObjects.exists(pbb->_obj_ref) || _gameObjects.exists(_gameObjects.get(pbb->_obj_ref)->attachedto))
        {
            is_invalid = true;
        }

        if (is_invalid)
        {
            // The billboard has expired:

            // Unlink it from the character and ..
            if (_gameObjects.exists(pbb->_obj_ref))
            {
                _gameObjects.get(pbb->_obj_ref)->ibillboard = INVALID_BBOARD_REF;
            }

            // ... and deallocate it.
            free_one(ref);
        }
        else
        {
            get_ptr(ref)->update();
        }
    }
}

void BillboardList::free_all()
{
    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        if (!lst[ref]._valid) continue;

        get_ptr(ref)->update();
    }
}

BBOARD_REF BillboardList::get_free_ref(Uint32 lifetime_secs)
{
    if (free_count <= 0)
    {
        return INVALID_BBOARD_REF;
    }

    if (0 == lifetime_secs)
    {
        return INVALID_BBOARD_REF;
    }


    TX_REF itex = TextureManager::get().acquire(INVALID_TX_REF);
    if (!VALID_TX_RANGE(itex))
    {
        return INVALID_BBOARD_REF;
    }

    BBOARD_REF ibb = INVALID_BBOARD_REF;
    size_t loops = 0;
    while (free_count > 0)
    {
        // grab the top index
        free_count--;
        update_guid++;

        ibb = free_ref[g_billboardList.free_count];

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

    if (VALID_BILLBOARD_RANGE(ibb))
    {
        billboard_data_t *pbb = get_ptr(ibb);

        pbb->init(true, SDL_GetTicks() + lifetime_secs * TICKS_PER_SEC, itex);
    }
    else
    {
        // the billboard allocation returned an ivaild value
        // deallocate the texture
        TextureManager::get().relinquish(itex);

        ibb = INVALID_BBOARD_REF;
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

bool billboard_system_begin()
{
    if (!_billboard_system_started)
    {
        g_billboardList.init_all();
        _billboard_system_started = true;
    }
    return _billboard_system_started;
}

bool billboard_system_end()
{
    if (_billboard_system_started)
    {
        g_billboardList.free_all();
        _billboard_system_started = false;
    }
    return !_billboard_system_started;
}

bool billboard_system_init()
{
    billboard_system_end();
    billboard_system_begin();

    return true;
}

bool billboard_system_render_one(billboard_data_t *pbb, float scale, const fvec3_t& cam_up, const fvec3_t& cam_rgt)
{
    if (NULL == pbb || !pbb->_valid) return false;

    if (!_gameObjects.exists(pbb->_obj_ref)) return false;

    // do not display for objects that are mounted or being held
    if (IS_ATTACHED_CHR_RAW(pbb->_obj_ref)) return false;

    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(pbb->_tex_ref);
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

gfx_rv billboard_system_render_all(std::shared_ptr<Camera> camera)
{
    if (!camera)
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    gfx_begin_3d(*camera);
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
                billboard_data_t *pbb = g_billboardList.get_ptr(ref);
                if (!pbb || !pbb->_valid ) continue;

                billboard_system_render_one(pbb, 0.75f, camera->getVUP(), camera->getVRT());
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();

    return gfx_success;
}


