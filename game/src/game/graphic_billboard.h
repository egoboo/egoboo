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
    using Colour3f = Ego::Math::Colour3f;
    using Colour4f = Ego::Math::Colour4f;
    bool _valid;        ///< has the billboard data been initialized?

    /**
     * @brief
     *  The point in time after which the billboard is expired.
     */
    Uint32 _endTime;
    /**
     * @brief
     *  The texture reference.
     */
    TX_REF _tex_ref;
    fvec3_t _position;          ///< the position of the bottom-missle of the box

    CHR_REF   _obj_ref;         ///< the character we are attached to

    /**
     * @brief
     *  The colour of the billboard.
     */
    Colour4f _tint;
    /**
     * @brief
     *  Additive over-time colour modifier.
     * @remark
     *  Each time the billboard is updated, <tt>_tint += _tint_add</tt> is computed.
     */
    fvec4_t _tint_add;

    /**
     * @brief
     *  An offset to the billboard's position.
     * @remark
     *  The offset is given in world cordinates.
     */
    fvec3_t _offset;
    /**
     * @brief
     *  Additive over-time offset modifier.
     * @remark
     *  Each time the billboard is updated, <tt>_offset += _offset_add</tt> is computed.
     */
    fvec3_t _offset_add;

    float _size;
    float _size_add;

    billboard_data_t();
    billboard_data_t *init(bool valid, Uint32 endTime, TX_REF tex_ref);
    billboard_data_t *init();
    bool free();
    bool update();
    bool printf_ttf(const std::shared_ptr<Ego::Font>& font, const Ego::Math::Colour4f& color, const char * format, ...) GCC_PRINTF_FUNC(4);

};


//--------------------------------------------------------------------------------------------
// BillboardList
//--------------------------------------------------------------------------------------------

struct BillboardList
{
private:
    Ego::GUID update_guid;
    int used_count;
    int free_count;
    size_t used_ref[BILLBOARDS_MAX];
    size_t free_ref[BILLBOARDS_MAX];
    billboard_data_t lst[BILLBOARDS_MAX];
public:
    void init_all();
    void update_all();
    void free_all();

    void clear_data();

    billboard_data_t *get_ptr(const size_t index)   {
        return LAMBDA(index >= BILLBOARDS_MAX, nullptr, &(lst[index]));
    }
    bool free_one(BBOARD_REF ref);
    /**
     * @brief
     *  Acquire a fresh billboard.
     * @param lifetime_secs
     *  the lifetime of the billboard, in seconds
     * @return
     *  the billboard reference on success, #INVALID_BBOARD_REF on failure.
     *  #INVALID_BBOARD_REF is also returned if there are no free billboards available or
     *  if no free texture reference could be acquired for the billboard. In particular,
     *  #INVALID_BBOARD_REF is also returned, if @a lifetime_secs is @a 0, as the billboard
     *  is already expired.
     */
    BBOARD_REF get_free_ref(Uint32 lifetime_secs);
};

extern BillboardList g_billboardList;

inline bool VALID_BILLBOARD_RANGE(BBOARD_REF ref)
{
    return (ref >= 0)
        && (ref < BILLBOARDS_MAX);
}

inline bool VALID_BILLBOARD(BBOARD_REF ref)
{
    return VALID_BILLBOARD_RANGE(ref)
        && g_billboardList.get_ptr(ref)->_valid;
}




//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool billboard_system_begin();
bool billboard_system_end();
bool billboard_system_init();

bool billboard_system_render_one(billboard_data_t *pbb, float scale, const fvec3_t& cam_up, const fvec3_t& cam_rgt);
gfx_rv billboard_system_render_all(std::shared_ptr<Camera> camera);
