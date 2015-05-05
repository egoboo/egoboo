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

/// @file game/graphic_billboard.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/graphic.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

// Forward declarations.
class Camera;
namespace Ego { class Font; }

//--------------------------------------------------------------------------------------------
// constants
//--------------------------------------------------------------------------------------------

enum e_bb_opt
{
    bb_opt_none          = EMPTY_BIT_FIELD,
    bb_opt_randomize_pos = ( 1 << 0 ),      // Randomize the position of the bb to witin 1 grid
    bb_opt_randomize_vel = ( 1 << 1 ),      // Randomize the velocity of the bb. Enough to move it by 2 tiles within its lifetime.
    bb_opt_fade          = ( 1 << 2 ),      // Make the billboard fade out
    bb_opt_burn          = ( 1 << 3 ),      // Make the tint fully saturate over time.
    bb_opt_all           = FULL_BIT_FIELD   // All of the above
};

//--------------------------------------------------------------------------------------------
// billboard_data_t
//--------------------------------------------------------------------------------------------

/// Description of a generic bilboarded object.
/// Any graphics that can be composited onto a SDL_surface can be used
struct billboard_data_t
{
    bool    valid;        ///< has the billboard data been initialized?

    Uint32    time;         ///< the time when the billboard will expire
    TX_REF    tex_ref;      ///< our texture index
    fvec3_t   pos;          ///< the position of the bottom-missle of the box

    CHR_REF   ichr;         ///< the character we are attached to

    GLXvector4f tint;       ///< a color to modulate the billboard's r,g,b, and a channels
    GLXvector4f tint_add;   ///< the change in tint per update

    GLXvector4f offset;     ///< an offset to the billboard's position in world coordinates
    GLXvector4f offset_add; ///< a "velocity vector" for the offest to make the billboard fly away

    float       size;
    float       size_add;

    static billboard_data_t *init(billboard_data_t *self);
    static bool free(billboard_data_t *self);
    static bool update(billboard_data_t *self);
    static bool printf_ttf(billboard_data_t *self, const std::shared_ptr<Ego::Font>& font, const Ego::Math::Colour4f& color, const char * format, ...) GCC_PRINTF_FUNC(4);

};


//--------------------------------------------------------------------------------------------
// BillboardList
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN(billboard_data_t, BillboardList, BILLBOARDS_MAX);

inline bool VALID_BILLBOARD_RANGE(BBOARD_REF ref)
{
    return (ref >= 0)
        && (ref < BILLBOARDS_MAX);
}

inline bool VALID_BILLBOARD(BBOARD_REF ref)
{
    return VALID_BILLBOARD_RANGE(ref)
        && BillboardList.lst[ref].valid;
}

void BillboardList_init_all();
void BillboardList_update_all();
void BillboardList_free_all();
BBOARD_REF BillboardList_get_free_ref(Uint32 lifetime_secs);
bool BillboardList_free_one(BBOARD_REF ref);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool billboard_system_begin();
bool billboard_system_end();
bool billboard_system_init();

bool billboard_system_render_one(billboard_data_t *pbb, float scale, const fvec3_t& cam_up, const fvec3_t& cam_rgt);
gfx_rv billboard_system_render_all(std::shared_ptr<Camera> camera);
