#pragma once

#include "egolib/Entities/Forward.hpp"

namespace Ego::Physics
{

class ParticlePhysics
{
public:
	ParticlePhysics(Ego::Particle &particle);

	void updatePhysics();

    void detachFromPlatform();

private:
    void updateEnviroment();

    /// @brief
    /// A helper method to compute the next valid position of this particle.
    /// Collisions with the mesh are included in this step.
    void updateMovement();
    
    /// @brief
    /// A helper method to compute the next valid position of this particle.
    /// Collisions with the mesh are included in this step.
    void updateAttached();
    
    /// @brief
    /// A helper method to compute gravitational acceleration and buoyancy of this particle.
    void updateParticleSimpleGravity();
    
    void updateHoming();
    
    /// @brief
    /// Helper method to compute the friction of this particle with the floor.
    void updateFloorFriction();

    /**
    * @brief
    *	Update gravity influence caused to and by this Particle
    **/
    void updateGravity();

private:
	static constexpr float STOPBOUNCINGPART = 10.0f;        ///< To make particles stop bouncing

	Ego::Particle& _particle;
};

} //Ego::Physics
