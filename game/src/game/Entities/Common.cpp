#include "game/Entities/Common.hpp"

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

const Vector3f& PhysicsData::getOldVelocity() const
{
    return m_oldVelocity;
}

void PhysicsData::setOldVelocity(const Vector3f& oldVelocity)
{
    m_oldVelocity = oldVelocity;
}

const Vector3f& PhysicsData::getVelocity() const
{
    return m_velocity;
}

void PhysicsData::setVelocity(const Vector3f& velocity)
{
    m_velocity = velocity;
}
