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
#include "game/graphics/Camera.hpp"
#include "game/char.h"
#include "game/entities/_Include.hpp"
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
billboard_data_t * billboard_data_init( billboard_data_t * pbb )
{
    if ( NULL == pbb ) return pbb;

    BLANK_STRUCT_PTR( pbb )

    pbb->tex_ref = INVALID_TX_REF;
    pbb->ichr    = INVALID_CHR_REF;

    pbb->tint[RR] = pbb->tint[GG] = pbb->tint[BB] = pbb->tint[AA] = 1.0f;
    pbb->size = 1.0f;

    /*    pbb->tint_add[AA] -= 1.0f / 400.0f;

        pbb->size_add -= 1.0f / 400.0f;

        pbb->offset_add[ZZ] += 127 / 50.0f * 2.0f;
    */
    return pbb;
}

//--------------------------------------------------------------------------------------------
bool billboard_data_free( billboard_data_t * pbb )
{
    if ( NULL == pbb || !pbb->valid ) return false;

    // free any allocated texture
	TextureManager::getSingleton()->relinquish(pbb->tex_ref);

    billboard_data_init( pbb );

    return true;
}

//--------------------------------------------------------------------------------------------
bool billboard_data_update( billboard_data_t * pbb )
{
    fvec3_t     vup, pos_new;
    Object     * pchr;
    float       height, offset;

    if ( NULL == pbb || !pbb->valid ) return false;

    if ( !_gameObjects.exists( pbb->ichr ) ) return false;
    pchr = _gameObjects.get( pbb->ichr );

    // determine where the new position should be
    chr_getMatUp(pchr, vup);

    height = pchr->bump.height;
    offset = std::min( pchr->bump.height * 0.5f, pchr->bump.size );

    pos_new.x = pchr->getPosX() + vup.x * ( height + offset );
    pos_new.y = pchr->getPosY() + vup.y * ( height + offset );
    pos_new.z = pchr->getPosZ() + vup.z * ( height + offset );

    // allow the billboards to be a bit bouncy
    pbb->pos.x = pbb->pos.x * 0.5f + pos_new.x * 0.5f;
    pbb->pos.y = pbb->pos.y * 0.5f + pos_new.y * 0.5f;
    pbb->pos.z = pbb->pos.z * 0.5f + pos_new.z * 0.5f;

    pbb->size += pbb->size_add;

    pbb->tint[RR] += pbb->tint_add[RR];
    pbb->tint[GG] += pbb->tint_add[GG];
    pbb->tint[BB] += pbb->tint_add[BB];
    pbb->tint[AA] += pbb->tint_add[AA];

    pbb->offset[XX] += pbb->offset_add[XX];
    pbb->offset[YY] += pbb->offset_add[YY];
    pbb->offset[ZZ] += pbb->offset_add[ZZ];

    // automatically kill a billboard that is no longer useful
    if ( pbb->tint[AA] == 0.0f || pbb->size == 0.0f )
    {
        billboard_data_free( pbb );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool billboard_data_printf_ttf( billboard_data_t * pbb, Font *font, SDL_Color color, const char * format, ... )
{
    va_list args;

    int rv;
    oglx_texture_t * ptex;
    GLfloat loc_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if ( NULL == pbb || !pbb->valid ) return false;

    // release any existing texture in case there is an error
	ptex = TextureManager::getSingleton()->get_valid_ptr(pbb->tex_ref);
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
    BBOARD_REF cnt;

    for ( cnt = 0; cnt < MAX_BBOARD; cnt++ )
    {
        billboard_data_init( BillboardList_get_ptr( cnt ) );
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
		if ((ticks >= pbb->time) || (nullptr == TextureManager::getSingleton()->get_valid_ptr(pbb->tex_ref)))
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
            billboard_data_update( BillboardList_get_ptr( cnt ) );
        }
    }
}

//--------------------------------------------------------------------------------------------
void BillboardList_free_all()
{
    BBOARD_REF cnt;

    for ( cnt = 0; cnt < MAX_BBOARD; cnt++ )
    {
        if ( !BillboardList.lst[cnt].valid ) continue;

        billboard_data_update( BillboardList_get_ptr( cnt ) );
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

    itex = TextureManager::getSingleton()->acquire( INVALID_TX_REF );
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

        billboard_data_init( pbb );

        pbb->tex_ref = itex;
        pbb->time    = SDL_GetTicks() + lifetime_secs * TICKS_PER_SEC;
        pbb->valid   = true;
    }
    else
    {
        // the billboard allocation returned an ivaild value
        // deallocate the texture
		TextureManager::getSingleton()->relinquish(itex);

        ibb = INVALID_BBOARD_REF;
    }

    return ibb;
}

//--------------------------------------------------------------------------------------------
bool BillboardList_free_one( size_t ibb )
{
    billboard_data_t * pbb;

    if ( !VALID_BILLBOARD_RANGE( ibb ) ) return false;
    pbb = BillboardList_get_ptr( ibb );

    billboard_data_free( pbb );

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < BillboardList.free_count; cnt++ )
        {
            if ( ibb == BillboardList.free_ref[cnt] ) return false;
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

	oglx_texture_t *ptex = TextureManager::getSingleton()->get_valid_ptr(pbb->tex_ref);

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

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );                   // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
			Ego::Renderer::getSingleton()->setDepthTestEnabled(true);
            GL_DEBUG( glDepthFunc )( GL_ALWAYS );                 // GL_DEPTH_BUFFER_BIT

            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );                                  // GL_LIGHTING_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();                                // GL_ENABLE_BIT

            GL_DEBUG( glEnable )( GL_BLEND );                                     // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );      // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );                                // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                          // GL_COLOR_BUFFER_BIT

			Ego::Renderer::getSingleton()->setColour(Ego::Colour4f::WHITE);

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


