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

#include "game/graphic.h"

// Forward declarations.
class Camera;
namespace Ego { class Font; }

/**
 * @brief
 *  Supposed to be a generic billboard.
 *  Currently, it merely taxes a texture a allows for some flags for position, blending and motion.
 */
struct Billboard
{
    enum Flags
    {
        None = EMPTY_BIT_FIELD,
        /**
         * @brief
         *  Randomize the position of the billboard within one grid along all axes.
         */
        RandomPosition = (1 << 0),
        /**
         * @brief
         *  Randomize the velocity of the billboard along all axes.
         *  Enough to move it by two tiles within its lifetime.
         */
        RandomVelocity = (1 << 1),
        /**
         * @brief
         *  Make the billboard fade out over time.
         */
        Fade = (1 << 2),
        /**
         * @brief
         *  Make the tint fully satured over time.
         */
        Burn = (1 << 3),
        /**
         * @brief
         *  All of the above.
         */
        All = FULL_BIT_FIELD,
    };

    using Colour3f = Ego::Math::Colour3f;
    using Colour4f = Ego::Math::Colour4f;

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
	Vector3f _position;          ///< the position of the bottom-missle of the box

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
	Vector4f _tint_add;

    /**
     * @brief
     *  An offset to the billboard's position.
     * @remark
     *  The offset is given in world cordinates.
     */
	Vector3f _offset;
    /**
     * @brief
     *  Additive over-time offset modifier.
     * @remark
     *  Each time the billboard is updated, <tt>_offset += _offset_add</tt> is computed.
     */
	Vector3f _offset_add;

    float _size;
    float _size_add;

    Billboard(Uint32 endTime, std::shared_ptr<oglx_texture_t> texture);
    bool update(Uint32 ticks);

};


struct BillboardList
{
public:

    // List of used billboards.
    std::list<std::shared_ptr<Billboard>> _used;
private:
    void update(Uint32 ticks);
public:
    BillboardList();
    void update();
    void reset();

    bool hasBillboard(const Object& object) {
        for (const auto& bb : _used) {
            if (bb->_obj_wptr.lock().get() == &object) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief
     *  Create a billboard.
     * @param lifetime_secs
     *  the lifetime of the billboard, in seconds
     * @param texture
     *  a shared pointer to the billboard texture.
     *  Must not be a null pointer.
     * @param tint
     *  the tint
     * @param options
     *  the options
     * @return
     *  the billboard
     * @throw std::invalid_argument
     *  if @a texture is a null pointer
     * @remark
     *  The billboard is kept around as long as a reference to the billboard exists,
     *  however, it might exprire during that time.
     */
    std::shared_ptr<Billboard> makeBillboard(Uint32 lifetime_secs, std::shared_ptr<oglx_texture_t> texture, const Ego::Math::Colour4f& tint, const BIT_FIELD options);

};

struct BillboardSystem {
protected:
    static BillboardSystem *singleton;
    BillboardSystem();
    virtual ~BillboardSystem();
    bool render_one(Billboard& bb, float scale, const Vector3f& cam_up, const Vector3f& cam_rgt);
public:
    static void initialize();
    static void uninitialize();
    static BillboardSystem& get();
public:
    void reset();
public:
    void render_all(Camera& camera);
    BillboardList _billboardList;
	// A vertex type used by the billboard system.
	struct Vertex {
		float x, y, z;
		float s, t;
	};
	// A vertex buffer used by the billboard system.
    Ego::VertexBuffer _vertexBuffer;
};

std::shared_ptr<Billboard> chr_make_text_billboard(const CHR_REF ichr, const char * txt, const Ego::Math::Colour4f& text_color, const Ego::Math::Colour4f& tint, int lifetime_secs, const BIT_FIELD opt_bits);
