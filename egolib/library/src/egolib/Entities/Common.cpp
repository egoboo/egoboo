#include "egolib/Entities/Common.hpp"

PhysicsData::PhysicsData() :
    phys(),
    targetplatform_level(0.0f),
    targetplatform_ref(),
    onwhichplatform_ref(),
    onwhichplatform_update(0),
    m_oldVelocity(),
    m_velocity()
{}

void PhysicsData::reset(PhysicsData *self)
{
    (*self) = PhysicsData();
}

const Ego::Vector3f& PhysicsData::getOldVelocity() const
{
    return m_oldVelocity;
}

void PhysicsData::setOldVelocity(const Ego::Vector3f& oldVelocity)
{
    m_oldVelocity = oldVelocity;
}

const Ego::Vector3f& PhysicsData::getVelocity() const
{
    return m_velocity;
}

void PhysicsData::setVelocity(const Ego::Vector3f& velocity)
{
    m_velocity = velocity;
}
