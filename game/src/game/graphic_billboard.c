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

INSTANTIATE_LIST(, billboard_data_t, BillboardList, BILLBOARDS_MAX);

//--------------------------------------------------------------------------------------------
// private functions
//--------------------------------------------------------------------------------------------

static void BillboardList_clear_data();

//--------------------------------------------------------------------------------------------
// private variables
//--------------------------------------------------------------------------------------------

static bool _billboard_system_started = false;

//--------------------------------------------------------------------------------------------
// billboard_data_t IMPLEMENTATION
//--------------------------------------------------------------------------------------------

billboard_data_t *billboard_data_t::init(billboard_data_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    BLANK_STRUCT_PTR(self);

    self->tex_ref = INVALID_TX_REF;
    self->ichr = INVALID_CHR_REF;

    self->tint[RR] = self->tint[GG] = self->tint[BB] = self->tint[AA] = 1.0f;
    self->size = 1.0f;

    return self;
}

bool billboard_data_t::free(billboard_data_t *self)
{
    if (!self || !self->valid)
    {
        return false;
    }

    // Relinquish the texture.
    TextureManager::get().relinquish(self->tex_ref);

    billboard_data_t::init(self);

    return true;
}

bool billboard_data_t::update(billboard_data_t *self)
{
    if (!self || !self->valid)
    {
        return false;
    }

    if (!_gameObjects.exists(self->ichr))
    {
        return false;
    }
    
    Object *pchr = _gameObjects.get(self->ichr);

    // Determine where the new position should be.
    fvec3_t vup, pos_new;
    chr_getMatUp(pchr, vup);

    self->size += self->size_add;

    self->tint[RR] += self->tint_add[RR];
    self->tint[GG] += self->tint_add[GG];
    self->tint[BB] += self->tint_add[BB];
    self->tint[AA] += self->tint_add[AA];

    /// @todo Why is this disabled. It should be there.
    //pbb->offset[XX] += pbb->offset_add[XX];
    //pbb->offset[YY] += pbb->offset_add[YY];
    self->offset[ZZ] += self->offset_add[ZZ];

    // Automatically kill a billboard that is no longer useful.
    if (self->tint[AA] <= 0.0f || self->size <= 0.0f)
    {
        billboard_data_t::free(self);
    }

    return true;
}

bool billboard_data_t::printf_ttf(billboard_data_t *self, const std::shared_ptr<Ego::Font> &font, const Ego::Math::Colour4f& color, const char * format, ...)
{
    Ego::Math::Colour3f fontColour = Ego::Math::Colour3f(color.getRed(), color.getGreen(), color.getBlue());

    if (!self || !self->valid)
    {
        return false;
    }

    // release any existing texture in case there is an error
    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(self->tex_ref);
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

IMPLEMENT_LIST(billboard_data_t, BillboardList, BILLBOARDS_MAX);

void BillboardList_init_all()
{
    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        billboard_data_t::init(BillboardList_get_ptr(ref));
    }

    BillboardList_clear_data();
}

void BillboardList_update_all()
{
    Uint32 ticks = SDL_GetTicks();

    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        billboard_data_t *pbb = BillboardList_get_ptr(ref);

        if (!pbb->valid) continue;

        bool is_invalid = false;
		if ((ticks >= pbb->time) || (nullptr == TextureManager::get().get_valid_ptr(pbb->tex_ref)))
        {
            is_invalid = true;
        }

        if (!_gameObjects.exists(pbb->ichr) || _gameObjects.exists(_gameObjects.get(pbb->ichr)->attachedto))
        {
            is_invalid = true;
        }

        if (is_invalid)
        {
            // The billboard has expired:

            // Unlink it from the character and ..
            if (_gameObjects.exists(pbb->ichr))
            {
                _gameObjects.get(pbb->ichr)->ibillboard = INVALID_BBOARD_REF;
            }

            // ... and deallocate it.
            BillboardList_free_one(ref);
        }
        else
        {
            billboard_data_t::update(BillboardList_get_ptr(ref));
        }
    }
}

void BillboardList_free_all()
{
    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        if (!BillboardList.lst[ref].valid) continue;

        billboard_data_t::update(BillboardList_get_ptr(ref));
    }
}

BBOARD_REF BillboardList_get_free_ref(Uint32 lifetime_secs)
{
    if (BillboardList.free_count <= 0)
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
    while (BillboardList.free_count > 0)
    {
        // grab the top index
        BillboardList.free_count--;
        BillboardList.update_guid++;

        ibb = BillboardList.free_ref[BillboardList.free_count];

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
        billboard_data_t *pbb = BillboardList_get_ptr(ibb);

        billboard_data_t::init(pbb);

        pbb->tex_ref = itex;
        pbb->time    = SDL_GetTicks() + lifetime_secs * TICKS_PER_SEC;
        pbb->valid   = true;
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

bool BillboardList_free_one(BBOARD_REF ref)
{
    if (!VALID_BILLBOARD_RANGE(ref)) return false;
    billboard_data_t *pbb = BillboardList_get_ptr(ref);
    billboard_data_t::free(pbb);

#if defined(_DEBUG)
    {
        // Determine whether this billboard is already in the list of free billboards.
        // If this is the case, then this is an error.
        for (BBOARD_REF i = 0; i < BillboardList.free_count; ++i)
        {
            if (ref == BillboardList.free_ref[i]) return false;
        }
    }
#endif

    if (BillboardList.free_count >= BILLBOARDS_MAX)
    {
        return false;
    }

    BillboardList.free_ref[BillboardList.free_count] = ref;
    BillboardList.free_count++;
    BillboardList.update_guid++;

    return true;
}

void BillboardList_clear_data()
{
    /// @author BB
    /// @details reset the free billboard list.

    for (BBOARD_REF ref = 0; ref < BILLBOARDS_MAX; ++ref)
    {
        BillboardList.free_ref[ref] = REF_TO_INT(ref);
    }

    BillboardList.free_count = BILLBOARDS_MAX;
}

//--------------------------------------------------------------------------------------------

bool billboard_system_begin()
{
    if (!_billboard_system_started)
    {
        BillboardList_init_all();
        _billboard_system_started = true;
    }
    return _billboard_system_started;
}

bool billboard_system_end()
{
    if (_billboard_system_started)
    {
        BillboardList_free_all();
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
    if (NULL == pbb || !pbb->valid) return false;

    if (!_gameObjects.exists( pbb->ichr)) return false;

    // do not display for objects that are mounted or being held
    if (IS_ATTACHED_CHR_RAW(pbb->ichr)) return false;

    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(pbb->tex_ref);
    oglx_texture_t::bind(ptex);


    float s = (float)ptex->getSourceWidth() / (float)ptex->getWidth();
    float t = (float)ptex->getSourceHeight() / (float)ptex->getHeight();

    // @todo this billboard stuff needs to be implemented as a OpenGL transform

    // scale the camera vectors
    fvec3_t vec_rgt = cam_rgt * (ptex->getSourceWidth() * scale * pbb->size);
    fvec3_t vec_up = cam_up  * (ptex->getSourceHeight() * scale * pbb->size);

    GLvertex vtlist[4];
    // bottom left
    fvec3_t tmp;
    tmp = pbb->pos + (-vec_rgt - vec_up * 0);
    vtlist[0].pos[XX] = pbb->offset[XX] + tmp[kX];
    vtlist[0].pos[YY] = pbb->offset[YY] + tmp[kY];
    vtlist[0].pos[ZZ] = pbb->offset[ZZ] + tmp[kZ];
    vtlist[0].tex[SS] = s;
    vtlist[0].tex[TT] = t;

    // top left
    tmp = pbb->pos + (-vec_rgt + vec_up * 2);
    vtlist[1].pos[XX] = pbb->offset[XX] + tmp[kX];
    vtlist[1].pos[YY] = pbb->offset[YY] + tmp[kY];
    vtlist[1].pos[ZZ] = pbb->offset[ZZ] + tmp[kZ];
    vtlist[1].tex[SS] = s;
    vtlist[1].tex[TT] = 0;

    // top right
    tmp = pbb->pos + (vec_rgt + vec_up * 2);
    vtlist[2].pos[XX] = pbb->offset[XX] + tmp[kX];
    vtlist[2].pos[YY] = pbb->offset[YY] + tmp[kY];
    vtlist[2].pos[ZZ] = pbb->offset[ZZ] + tmp[kZ];
    vtlist[2].tex[SS] = 0;
    vtlist[2].tex[TT] = 0;

    // bottom right
    tmp = pbb->pos + (vec_rgt - vec_up * 0);
    vtlist[3].pos[XX] = pbb->offset[XX] + tmp[kX];
    vtlist[3].pos[YY] = pbb->offset[YY] + tmp[kY];
    vtlist[3].pos[ZZ] = pbb->offset[ZZ] + tmp[kZ];
    vtlist[3].tex[SS] = 0;
    vtlist[3].tex[TT] = t;

    {
        auto& renderer = Ego::Renderer::get();
        renderer.setColour(Ego::Math::Colour4f(pbb->tint[RR], pbb->tint[GG], pbb->tint[BB], pbb->tint[AA]));

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
                billboard_data_t *pbb = BillboardList_get_ptr(ref);
                if (!pbb || !pbb->valid ) continue;

                billboard_system_render_one(pbb, 0.75f, camera->getVUP(), camera->getVRT());
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();

    return gfx_success;
}


