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
#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/Graphics/Camera.hpp"
#include "game/char.h"
#include "game/Entities/_Include.hpp"
#include "game/game.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST(, billboard_data_t, BillboardList, BILLBOARD_COUNT );

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

bool billboard_data_t::free(billboard_data_t * self)
{
    if (!self || !self->valid)
    {
        return false;
    }

    // free any allocated texture
    TextureManager::get().relinquish(self->tex_ref);

    billboard_data_t::init(self);

    return true;
}

bool billboard_data_t::update(billboard_data_t *self)
{
    fvec3_t     vup, pos_new;

    if (!self || !self->valid)
    {
        return false;
    }

    if (!_gameObjects.exists(self->ichr))
    {
        return false;
    }
    Object *pchr = _gameObjects.get(self->ichr);

    // determine where the new position should be
    chr_getMatUp(pchr, vup);

    self->size += self->size_add;

    self->tint[RR] += self->tint_add[RR];
    self->tint[GG] += self->tint_add[GG];
    self->tint[BB] += self->tint_add[BB];
    self->tint[AA] += self->tint_add[AA];

    self->offset[ZZ] += self->offset_add[ZZ];

    // automatically kill a billboard that is no longer useful
    if (self->tint[AA] <= 0.0f || self->size <= 0.0f)
    {
        billboard_data_t::free(self);
    }

    return true;
}

bool billboard_data_t::printf_ttf(billboard_data_t *self, Font *font, SDL_Color color, const char * format, ...)
{
    va_list args;

    int rv;
    oglx_texture_t * ptex;
    GLfloat loc_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (!self || !self->valid) return false;

    // release any existing texture in case there is an error
    ptex = TextureManager::get().get_valid_ptr(self->tex_ref);
    oglx_texture_release( ptex );

    va_start( args, format );
    rv = fnt_vprintf_OGL( font, color, ptex->base.binding, loc_coords, format, args, &( ptex->surface ) );
    va_end( args );

    ptex->base_valid = false;
    oglx_grab_texture_state( GL_TEXTURE_2D, 0, ptex );

    ptex->imgW  = ptex->surface->w;
    ptex->imgH  = ptex->surface->h;
    strncpy( ptex->name, "billboard text", SDL_arraysize( ptex->name ) );

    return ( rv >= 0 );
}

//--------------------------------------------------------------------------------------------
// BillboardList IMPLEMENTATION
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( billboard_data_t, BillboardList, MAX_BBOARD );

//--------------------------------------------------------------------------------------------
void BillboardList_init_all()
{
    for (BBOARD_REF ref = 0; ref < MAX_BBOARD; ++ref)
    {
        billboard_data_t::init(BillboardList_get_ptr( ref));
    }

    BillboardList_clear_data();
}

//--------------------------------------------------------------------------------------------
void BillboardList_update_all()
{
    BBOARD_REF cnt;
    Uint32     ticks;

    ticks = SDL_GetTicks();

    for ( cnt = 0; cnt < MAX_BBOARD; cnt++ )
    {
        bool is_invalid;

        billboard_data_t * pbb = BillboardList_get_ptr( cnt );

        if ( !pbb->valid ) continue;

        is_invalid = false;
		if ((ticks >= pbb->time) || (nullptr == TextureManager::get().get_valid_ptr(pbb->tex_ref)))
        {
            is_invalid = true;
        }

        if ( !_gameObjects.exists( pbb->ichr ) || _gameObjects.exists( _gameObjects.get(pbb->ichr)->attachedto ) )
        {
            is_invalid = true;
        }

        if ( is_invalid )
        {
            // the billboard has expired

            // unlink it from the character
            if ( _gameObjects.exists( pbb->ichr ) )
            {
                _gameObjects.get(pbb->ichr)->ibillboard = INVALID_BBOARD_REF;
            }

            // deallocate the billboard
            BillboardList_free_one( REF_TO_INT( cnt ) );
        }
        else
        {
            billboard_data_t::update( BillboardList_get_ptr( cnt ) );
        }
    }
}

//--------------------------------------------------------------------------------------------
void BillboardList_free_all()
{
    for (BBOARD_REF ref = 0; ref < MAX_BBOARD; ++ref)
    {
        if (!BillboardList.lst[ref].valid)
        {
            continue;
        }
        billboard_data_t::update(BillboardList_get_ptr(ref));
    }
}

//--------------------------------------------------------------------------------------------
size_t BillboardList_get_free_ref( Uint32 lifetime_secs )
{
    TX_REF             itex  = INVALID_TX_REF;
    billboard_data_t * pbb   = NULL;
    size_t             ibb   = INVALID_BBOARD_REF;
    size_t             loops = 0;

    if ( BillboardList.free_count <= 0 ) return INVALID_BBOARD_REF;

    if ( 0 == lifetime_secs ) return INVALID_BBOARD_REF;

    itex = TextureManager::get().acquire( INVALID_TX_REF );
    if ( !VALID_TX_RANGE( itex ) ) return INVALID_BBOARD_REF;

    while ( BillboardList.free_count > 0 )
    {
        // grab the top index
        BillboardList.free_count--;
        BillboardList.update_guid++;

        ibb = BillboardList.free_ref[BillboardList.free_count];

        if ( VALID_BILLBOARD_RANGE( ibb ) )
        {
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the free stack. %d loops.\n", __FUNCTION__, loops );
    }

    if ( VALID_BILLBOARD_RANGE( ibb ) )
    {
        pbb = BillboardList_get_ptr( ibb );

        billboard_data_t::init( pbb );

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

//--------------------------------------------------------------------------------------------
bool BillboardList_free_one( size_t ibb )
{
    if (!VALID_BILLBOARD_RANGE(ibb)) return false;
    billboard_data_t *pbb = BillboardList_get_ptr(ibb);

    billboard_data_t::free(pbb);

#if defined(_DEBUG)
    {
        // If the billboard is already in the list of free billboards,
        // then this is an error.
        for (size_t i = 0; i < BillboardList.free_count; ++i)
        {
            if (ibb == BillboardList.free_ref[i]) return false;
        }
    }
#endif

    if ( BillboardList.free_count >= MAX_BBOARD )
        return false;

    // do not put anything below TX_SPECIAL_LAST back onto the SDL_free stack
    BillboardList.free_ref[BillboardList.free_count] = ibb;

    BillboardList.free_count++;
    BillboardList.update_guid++;

    return true;
}

//--------------------------------------------------------------------------------------------

void BillboardList_clear_data()
{
    /// @author BB
    /// @details reset the free billboard list.

    for (BBOARD_REF ref = 0; ref < MAX_BBOARD; ++ref)
    {
        BillboardList.free_ref[ref] = REF_TO_INT(ref);
    }

    BillboardList.free_count = MAX_BBOARD;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool billboard_system_begin()
{
    if ( !_billboard_system_started )
    {

        BillboardList_init_all();

        _billboard_system_started = true;
    }

    return _billboard_system_started;
}

//--------------------------------------------------------------------------------------------
bool billboard_system_end()
{
    if ( _billboard_system_started )
    {
        BillboardList_free_all();

        _billboard_system_started = false;
    }

    return !_billboard_system_started;

}

//--------------------------------------------------------------------------------------------
bool billboard_system_init()
{
    billboard_system_end();
    billboard_system_begin();

    return true;
}

//--------------------------------------------------------------------------------------------
bool billboard_system_render_one( billboard_data_t * pbb, float scale, const fvec3_t& cam_up, const fvec3_t& cam_rgt )
{
    int i;
    GLvertex vtlist[4];
    float x1, y1;

    if (NULL == pbb || !pbb->valid) return false;

    if (!_gameObjects.exists( pbb->ichr)) return false;

    // do not display for objects that are mounted or being held
    if (IS_ATTACHED_CHR_RAW(pbb->ichr)) return false;

	oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(pbb->tex_ref);

    oglx_texture_bind(ptex);

	float w = oglx_texture_getImageWidth(ptex);
    float h = oglx_texture_getImageHeight(ptex);

    x1 = w  / (float)oglx_texture_t::getTextureWidth(ptex);
    y1 = h  / (float)oglx_texture_t::getTextureHeight(ptex);

    // @todo this billboard stuff needs to be implemented as a OpenGL transform

    // scale the camera vectors
	fvec3_t vec_rgt = cam_rgt * (w * scale * pbb->size);
	fvec3_t vec_up  = cam_up  * (h * scale * pbb->size);

    // bottom left
    vtlist[0].pos[XX] = pbb->offset[XX] + pbb->pos.x + ( -vec_rgt.x - 0 * vec_up.x );
    vtlist[0].pos[YY] = pbb->offset[YY] + pbb->pos.y + ( -vec_rgt.y - 0 * vec_up.y );
    vtlist[0].pos[ZZ] = pbb->offset[ZZ] + pbb->pos.z + ( -vec_rgt.z - 0 * vec_up.z );
    vtlist[0].tex[SS] = x1;
    vtlist[0].tex[TT] = y1;

    // top left
    vtlist[1].pos[XX] = pbb->offset[XX] + pbb->pos.x + ( -vec_rgt.x + 2 * vec_up.x );
    vtlist[1].pos[YY] = pbb->offset[YY] + pbb->pos.y + ( -vec_rgt.y + 2 * vec_up.y );
    vtlist[1].pos[ZZ] = pbb->offset[ZZ] + pbb->pos.z + ( -vec_rgt.z + 2 * vec_up.z );
    vtlist[1].tex[SS] = x1;
    vtlist[1].tex[TT] = 0;

    // top right
    vtlist[2].pos[XX] = pbb->offset[XX] + pbb->pos.x + ( vec_rgt.x + 2 * vec_up.x );
    vtlist[2].pos[YY] = pbb->offset[YY] + pbb->pos.y + ( vec_rgt.y + 2 * vec_up.y );
    vtlist[2].pos[ZZ] = pbb->offset[ZZ] + pbb->pos.z + ( vec_rgt.z + 2 * vec_up.z );
    vtlist[2].tex[SS] = 0;
    vtlist[2].tex[TT] = 0;

    // bottom right
    vtlist[3].pos[XX] = pbb->offset[XX] + pbb->pos.x + ( vec_rgt.x - 0 * vec_up.x );
    vtlist[3].pos[YY] = pbb->offset[YY] + pbb->pos.y + ( vec_rgt.y - 0 * vec_up.y );
    vtlist[3].pos[ZZ] = pbb->offset[ZZ] + pbb->pos.z + ( vec_rgt.z - 0 * vec_up.z );
    vtlist[3].tex[SS] = 0;
    vtlist[3].tex[TT] = y1;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    {
        // Go on and draw it
        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glColor4fv )( pbb->tint );

            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                GL_DEBUG( glVertex3fv )( vtlist[i].pos );
            }
        }
        GL_DEBUG_END();
    }
    ATTRIB_POP( __FUNCTION__ );

    return true;
}

//--------------------------------------------------------------------------------------------
gfx_rv billboard_system_render_all( std::shared_ptr<Camera> pcam )
{
    BBOARD_REF cnt;

    if ( nullptr == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    gfx_begin_3d( pcam );
    {
        ATTRIB_PUSH( __FUNCTION__, GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
        {
            billboard_data_t * pbb;

            // Do not write into the depth buffer.
            Ego::Renderer::get().setDepthWriteEnabled(false);

            // Essentially disable the depth test without calling
            // Ego::Renderer::get().setDepthTestEnabled(false).
			Ego::Renderer::get().setDepthTestEnabled(true);
            Ego::Renderer::get().setDepthFunction(Ego::CompareFunction::AlwaysPass);

            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );                                  // GL_LIGHTING_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();                                // GL_ENABLE_BIT

            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );      // GL_COLOR_BUFFER_BIT

            Ego::Renderer::get().setAlphaTestEnabled(true);
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                          // GL_COLOR_BUFFER_BIT

			Ego::Renderer::get().setColour(Ego::Colour4f::WHITE);

            for ( cnt = 0; cnt < MAX_BBOARD; cnt++ )
            {
                pbb = BillboardList_get_ptr( cnt );
                if ( NULL == pbb || !pbb->valid ) continue;

                billboard_system_render_one(pbb, 0.75f, pcam->getVUP(), pcam->getVRT());
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();

    return gfx_success;
}


