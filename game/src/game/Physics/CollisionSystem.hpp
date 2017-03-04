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

#include "idlib/idlib.hpp"
#include "egolib/egolib.h"

//Forward declarations
namespace Ego { class Particle; }

namespace Ego
{
namespace Physics
{

class CollisionSystem : public Core::Singleton<CollisionSystem>
{
public:
    /**
    * @brief
    *   Detect and handle all Object to Object collisions
    **/
    void updateObjectCollisions();

    /**
    * @brief
    *   Detect and handle all Particle to Object collisions
    **/
    void updateParticleCollisions();

    void update();

private:
    /**
    * @brief
    *   Detects if a collision occurs between two Objects
    * @param objectA
    *   The first of the two objects which maybe collides with objectB
    * @param objectB
    *   The Object that objectA maybe collides with
    * @return
    *   true if these two Object actually collide, false otherwise
    **/
    bool detectCollision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB, float *tmin, float *tmax) const;

    /**
    * @brief
    *   Detects if a collision occurs between a Particle and an Object
    * @param particle
    *   The Particle which maybe collides with an Object
    * @param object
    *   The Object which maybe collides with the Particle
    * @return
    *   true if these two Entities actually collide, false otherwise
    **/
    bool detectCollision(const std::shared_ptr<Ego::Particle> &particle, const std::shared_ptr<Object> &object, float *tmin, float *tmax) const;

    /**
    * @brief
    *   Actually handles and resolves a collision between two Objects
    * @param objectA
    *   The first of the two objects which collides with objectB
    * @param objectB
    *   The object that objectA collides with
    **/
    void handleCollision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB, const float tmin, const float tmax);

    /**
    * @brief
    *   Handles collision interaction between an Object and a platform Object
    **/
    bool handlePlatformCollision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB);

    /**
    * @brief
    *   Makes an Object mount and ride another Object
    * @param character
    *   The rider
    * @param mount
    *   The Object which character is trying to ride
    * @return
    *   true if character is now riding mount
    **/
    bool handleMountingCollision(const std::shared_ptr<Object> &character, const std::shared_ptr<Object> &mount);

private:
    friend Core::Singleton<CollisionSystem>::CreateFunctorType;
    friend Core::Singleton<CollisionSystem>::DestroyFunctorType;
    CollisionSystem();
    ~CollisionSystem();
};

} //namespace Physics
} //namespace Ego
