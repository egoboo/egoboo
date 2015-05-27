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
    std::shared_ptr<oglx_texture_t> _texture;
    fvec3_t _position;          ///< the position of the bottom-missle of the box

    /**
     * @brief
     *  The object this billboard is attached to.
     */
    std::weak_ptr<Object> _obj_wptr;

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
    void set(bool valid, Uint32 endTime, std::shared_ptr<oglx_texture_t> texture);
    void reset();
    void free();
    bool update();

};


//--------------------------------------------------------------------------------------------
// BillboardList
//--------------------------------------------------------------------------------------------

bool VALID_BILLBOARD_RANGE(BBOARD_REF ref);

bool VALID_BILLBOARD(BBOARD_REF ref);

struct BillboardList
{
private:
    Ego::GUID update_guid;
    int used_count;
    int free_count;
    BBOARD_REF used_ref[BILLBOARDS_MAX];
    BBOARD_REF free_ref[BILLBOARDS_MAX];
    billboard_data_t lst[BILLBOARDS_MAX];
    void clear_data();
public:
    BillboardList();
    void update();
    void reset();

    bool hasBillboard(const Object& object) {
        for (BBOARD_REF i(0); i; ++i) {
            billboard_data_t *bb_ptr = &(lst[i]);
            if (bb_ptr->_valid) {
                if (bb_ptr->_obj_wptr.lock().get() == &object) {
                    return true;
                }
            }
        }
        return false;
    }

    billboard_data_t *get_ptr(const BBOARD_REF ref)   {
        return LAMBDA(!VALID_BILLBOARD_RANGE(ref), nullptr, &(lst[ref]));
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
    BBOARD_REF get_free_ref(Uint32 lifetime_secs, std::shared_ptr<oglx_texture_t> texture, const Ego::Math::Colour4f& tint, const BIT_FIELD opt_bits);
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct BillboardSystem {
protected:
    static BillboardSystem *singleton;
    BillboardSystem();
    virtual ~BillboardSystem();
    bool render_one(billboard_data_t *pbb, float scale, const fvec3_t& cam_up, const fvec3_t& cam_rgt);
public:
    static void initialize();
    static void uninitialize();
    static BillboardSystem& get();
public:
    void reset();
public:
    void render_all(Camera& camera);
    BillboardList _billboardList;
    std::shared_ptr<Ego::VertexBuffer> _vertexBuffer;
};
