#pragma once

#include <IdLib/IdLib.hpp>
#include <egolib/egolib.h>

//Forward declarations
namespace Ego { class Particle; }

namespace Ego
{
namespace Physics
{

class CollisionSystem : public Ego::Core::Singleton<CollisionSystem>
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
    bool detectCollision(const std::shared_ptr<Ego::Particle> &particle, const std::shared_ptr<Object> &object) const;

    /**
    * @brief
    *   Actually handles and resolves a collision between two Objects
    * @param objectA
    *   The first of the two objects which collides with objectB
    * @param objectB
    *   The object that objectA collides with
    **/
    void handleCollision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB, const float tmin, const float tmax);

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

    bool attachObjectToPlatform(const std::shared_ptr<Object> &object, const std::shared_ptr<Object> &platform);

protected:

    // Befriend with singleton to grant access to ParticleProfileSystem::CollisionSystem and CollisionSystem::~CollisionSystem.
    using TheSingleton = Ego::Core::Singleton<CollisionSystem>;
    friend TheSingleton;

    CollisionSystem();
    ~CollisionSystem();
};

} //namespace Physics
} //namespace Ego
