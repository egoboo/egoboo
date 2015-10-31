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

    PhysicsData() :
        phys(),
        targetplatform_level(0.0f),
        targetplatform_ref(INVALID_CHR_REF),
        onwhichplatform_ref(INVALID_CHR_REF),
        onwhichplatform_update(0),
        vel_old(), 
        vel()
    {
        // initialize the physics
        phys_data_t::ctor(&phys);
    }
    
    static void reset(PhysicsData *self)
    {
        phys_data_t::reset(&self->phys);

        self->targetplatform_level = 0.0f;
        self->targetplatform_ref = INVALID_CHR_REF;
        self->onwhichplatform_ref = INVALID_CHR_REF;
        self->onwhichplatform_update = 0;

        self->vel = Vector3f::zero();
        self->vel_old = Vector3f::zero();
    }
};
