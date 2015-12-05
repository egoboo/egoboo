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

    /**
    * @return
    *   Vector containing the desired XY velocity of the object
    **/
    const Vector2f& getDesiredVelocity() const;

private:
    void updateMovement(const std::shared_ptr<Object> &object);
    
    /**
    * @brief
    *   This makes objects slide down hillsides
    **/
    void updateHillslide(const std::shared_ptr<Object> &pchr);

    /**
    * @brief
    *   This updates which way the Object is looking based on movement
    **/
    void updateFacing(const std::shared_ptr<Object> &pchr);

    /**
    * @return
    *   The actual max velocity of an Object including all abilities, special effects
    *   and any other speed modifiers
    **/
    float getMaxSpeed(Object *object) const;
    
    /// @author BB
    /// @details Figure out the next position of the character.
    ///    Include collisions with the mesh in this step.
    void updateMeshCollision(const std::shared_ptr<Object> &pchr);

    /**
    * @brief
    *   This makes objects interact with the platform they are attached to (if any)
    **/
    void updatePlatformPhysics(const std::shared_ptr<Object> &object);

    /**
    * @brief
    *   This keeps held items in the hands of their holder and updates
    *   transfer blending
    **/
    void keepItemsWithHolder(const std::shared_ptr<Object> &pchr);

private:
    static constexpr float MAX_DISPLACEMENT_XY = 20.0f;     //< Max velocity correction due to being inside a wall

    Vector2f _platformOffset;
    Vector2f _desiredVelocity;
    float _traction;
};

} //Physics
} //Ego
