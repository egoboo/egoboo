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

#include "egolib/game/mesh.h"
#include "egolib/game/physics.h"

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
    /// @brief The entity's physics data.
    phys_data_t phys;
    /// @brief What is the height of the target platform?
    float targetplatform_level;  
    /// @brief Am I trying to attach to a platform?
    ObjectRef targetplatform_ref;
    /// @brief Is the particle on a platform?
    ObjectRef onwhichplatform_ref;
    /// @brief When was the last platform attachment made?
    uint32_t onwhichplatform_update;

private:
    /// @brief The old velocity of the entity.
    Vector3f m_oldVelocity;
    /// @brief The current velocity of the entity.
    Vector3f m_velocity;
public:
    PhysicsData();

    static void reset(PhysicsData *self);

    /// @brief Get the old velocity.
    /// @return the old velocity
    const Vector3f& getOldVelocity() const;

    /// @brief Set the old velocity.
    /// @param oldVelocity the old velocity
    void setOldVelocity(const Vector3f& oldVelocity);
     
    /// @brief Get the velocity.
    /// @return the velocity
    const Vector3f& getVelocity() const;

    /// @brief Set the velocity.
    /// @param velocity the velocity
    void setVelocity(const Vector3f& velocity);
};
