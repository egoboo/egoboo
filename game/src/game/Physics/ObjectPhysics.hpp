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

/// @file  game/Physics/ObjectPhysics.hpp
/// @brief Code for handling object physics
/// @author Johan Jansen aka Zefz
#pragma once

#include "IdLib/IdLib.hpp"
#include "egolib/egolib.h"

//Forward declarations
class Object;

namespace Ego
{
namespace Physics
{

class ObjectPhysics
{
public:
    ObjectPhysics();

    /**
    * @brief
    *   Update a single physics tick for this Object.
    *   This integrates acceleration and velocities into new positions and
    *   includes collisions with the mesh.
    **/
    void updatePhysics(const std::shared_ptr<Object> &pchr);

    /// @author BB
    /// @details attach a character to a platform
    ///
    /// @note the function move_one_character_get_environment() has already been called from within the
    ///  move_one_character() function, so the environment has already been determined this round
    void detachFromPlatform(Object* pchr);

    bool attachToPlatform(const std::shared_ptr<Object> &object, const std::shared_ptr<Object> &platform);

private:
    void updateMovement(const std::shared_ptr<Object> &object);
    void updateFriction(const std::shared_ptr<Object> &pchr);
    float getMaxSpeed(Object *object) const;
    void updateFacing(const std::shared_ptr<Object> &pchr, const Vector2f &desiredVelocity);

    void updatePlatformPhysics(const std::shared_ptr<Object> &object);

private:
    Vector2f _platformOffset;
};

} //Physics
} //Ego
