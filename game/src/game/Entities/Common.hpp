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
#pragma once

#include "game/mesh.h"

/**
 * @brief
 *  A count-down timer.
 * @author
 *  Michael Heilmann
 */
struct CountDownTimer
{

private:

    /**
     * @brief
     *  The length of the count-down e.g. 20 seconds.
     */
    unsigned long _length;

    /**
     * @brief
     *  The remaining time e.g. 5 seconds.
     */
    unsigned long _remaining;

public:

    /**
     * @brief
     *  Construct this count-down timer.
     * @param length
     *  the length of the count-down. Default value is @a 0.
     * @post
     *  The count-down length is @a length.
     *  The rmaining time is @a length.
     */
    CountDownTimer(unsigned long length = 0) :
        _length(length), _remaining(length)
    {}

    /**
     * @brief
     *  Reset this count-down timer.
     * @param length
     *  the length of the count-down. default value is @a 0.
     * @post
     *  The count-down length is @a length.
     *  The rmaining time is @a length.
     */
    void reset(unsigned long length = 0)
    {
        _length = length;
        _remaining = length;
    }

    /**
     * @brief
     *  Notify the timer, that some time has passed.
     * @param time
     *  the time which has passed
     */
    void timePassed(unsigned long time)
    {
        _remaining -= std::min(_remaining, time);
    }

    /**
     * @brief
     *  Get the count-down length.
     * @return
     *  the count-down length
     */
    unsigned long getLength() const
    {
        return _length;
    }

    /**
     * @brief
     *  Get the elapsed time.
     * @return
     *  the elapsed time
     */
    unsigned long getElapsed() const
    {
        return _length - _remaining;
    }

    /**
     * @brief
     *  Get the remaining time.
     * @return
     *  the remaining time
     */
    unsigned long getRemaining() const
    {
        return _remaining;
    }

};

struct PhysicsData
{
    phys_data_t phys;          ///< The entity's physics data.

    bool safe_valid;               ///< Is the last "safe" position valid?
	Vector3f safe_pos;             ///< The last "safe" position.
    Uint32 safe_time;              ///< The last "safe" time.
    TileIndex safe_grid;           ///< the last "safe" grid. @todo Should be of type @a TileIndex.

    /**
     * @brief
     *  The tile this object is on or TileIndex::Invalid if none.
     */
    TileIndex _tile;
    /**
     * @brief
     *  The block this object is on or BlockIndex::Invalid if none.
     */
    BlockIndex _block;

    float targetplatform_level;    ///< What is the height of the target platform?
    CHR_REF targetplatform_ref;    ///< Am I trying to attach to a platform?
    CHR_REF onwhichplatform_ref;   ///< Is the particle on a platform?
    Uint32 onwhichplatform_update; ///< When was the last platform attachment made?

    /**
    * @brief
    *  The current velocity of the entity.
    */
	Vector3f vel;
    /**
    * @brief
    *  The previous velocity of the entity.
    */
	Vector3f vel_old;

    /**
    * @brief
    *  The current position of the entity.
    */
	Vector3f pos;
    /**
    * @brief
    *  The previous position of the entity.
    */
	Vector3f pos_old;

    /**
    * @brief
    *  The initial/starting position.
    */
	Vector3f pos_stt;

    PhysicsData() :
        phys(),
        safe_valid(false), safe_pos(), safe_time(0), safe_grid(TileIndex::Invalid),
        _tile(TileIndex::Invalid), _block(BlockIndex::Invalid),
        targetplatform_level(0.0f),
        targetplatform_ref(INVALID_CHR_REF),
        onwhichplatform_ref(INVALID_CHR_REF),
        onwhichplatform_update(0),
        pos_old(), pos(),
        vel_old(), vel(),
        pos_stt()
    {
        // initialize the physics
        phys_data_t::ctor(&phys);
    }
    static void reset(PhysicsData *self)
    {
        phys_data_t::reset(&self->phys);

        self->safe_valid = false;
        self->safe_pos = Vector3f::zero();
        self->safe_time = 0;
        self->safe_grid = TileIndex::Invalid;

        self->_tile = TileIndex::Invalid;
        self->_block = BlockIndex::Invalid;

        self->targetplatform_level = 0.0f;
        self->targetplatform_ref = INVALID_CHR_REF;
        self->onwhichplatform_ref = INVALID_CHR_REF;
        self->onwhichplatform_update = 0;

        self->pos = Vector3f::zero();
        self->pos_old = Vector3f::zero();

        self->vel = Vector3f::zero();
        self->vel_old = Vector3f::zero();

        self->pos_stt = Vector3f::zero();
    }

    /**
     * @brief Get the tile this object is currently on.
     * @return the tile index of the tile this object is on.
     * If the object is currently on no tile, TileIndex::Invalid is returned.
     */
    inline const TileIndex& getTile() const {
        return _tile;
    }

    /**
     * @brief Get the block this object is currently on.
     * @return the block index of the block this object is on.
     * If the object is currently on no block, BlockIndex::Invalid is returned.
     */
    inline const BlockIndex& getBlock() const {
        return _block;
    }

    /**
     * @return the current position of this object
     */
    inline const Vector3f& getPosition() const {
        return pos;
    }

    /**
     * @return the position of this object along the x-axis
     */
    inline float getPosX() const {
        return pos[kX];
    }

    /**
     * @return the position of this object along the y-axis
     */
    inline float getPosY() const {
        return pos[kY];
    }

    /**
     * @return the position of this object along the z-axis
     */
    inline float getPosZ() const {
        return pos[kZ];
    }

	/// @brief Return nonzero if the entity hit a wall that the entity is not allowed to cross.
	virtual BIT_FIELD hit_wall(Vector2f& nrm, float *pressure, mesh_wall_data_t *data) = 0;
	/// @brief Returns nonzero if the entity hit a wall that the entity is not allowed to cross.
	virtual BIT_FIELD hit_wall(const Vector3f& pos, Vector2f& nrm, float *pressure, mesh_wall_data_t *data) = 0;
	/// @brief Returns nonzero if the entity hit a wall that the entity is not allowed to cross.
	virtual BIT_FIELD test_wall(mesh_wall_data_t *data) = 0;
	/// @brief Return nonzero if the entity hit a wall that the entity is not allowed to cross.
	virtual BIT_FIELD test_wall(const Vector3f& pos, mesh_wall_data_t *data) = 0;

};
