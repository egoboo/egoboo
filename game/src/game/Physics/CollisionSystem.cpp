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
#include "CollisionSystem.hpp"
#include "game/Entities/_Include.hpp"
#include "game/game.h" //for update_wld

#include "particle_collision.h"

namespace Ego
{
namespace Physics
{
//C function prototypes
static bool do_chr_chr_collision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB, float tmax, float tmin);
static void get_recoil_factors( float wta, float wtb, float * recoil_a, float * recoil_b );

CollisionSystem::CollisionSystem()
{

}


CollisionSystem::~CollisionSystem()
{

}

void CollisionSystem::update()
{
    // blank the accumulators
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        object->phys.clear();
    }
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        particle->phys.clear();
    }

    updateObjectCollisions();
    updateParticleCollisions();

    // accumulate the accumulators
    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        if(pchr->isTerminated()) {
            continue;
        }
        
        float tmpx, tmpy;
        bool position_updated = false;
        Vector3f max_apos;

		Vector3f tmp_pos = pchr->getPosition();

        // do the "integration" of the accumulated accelerations
        pchr->vel += pchr->phys.avel;

        // get a net displacement vector from aplat and acoll
        {
            // create a temporary apos_t
            apos_t  apos_tmp;

            // copy 1/2 of the data over
            apos_tmp = pchr->phys.aplat;

            // get the resultant apos_t
            apos_tmp.join(pchr->phys.acoll);

            // turn this into a vector
            apos_t::evaluate(apos_tmp, max_apos);
        }

        // limit the size of the displacement
        max_apos[kX] = Ego::Math::constrain( max_apos[kX], -Info<float>::Grid::Size(), Info<float>::Grid::Size());
        max_apos[kY] = Ego::Math::constrain( max_apos[kY], -Info<float>::Grid::Size(), Info<float>::Grid::Size());
        max_apos[kZ] = Ego::Math::constrain( max_apos[kZ], -Info<float>::Grid::Size(), Info<float>::Grid::Size());

        // do the "integration" on the position
        if (std::abs(max_apos[kX]) > 0.0f)
        {
            tmpx = tmp_pos[kX];
            tmp_pos[kX] += max_apos[kX];
            if ( EMPTY_BIT_FIELD != pchr->test_wall( tmp_pos ) )
            {
                // restore the old values
                tmp_pos[kX] = tmpx;
            }
            else
            {
                //pchr->vel[kX] += pchr->phys.apos_coll[kX] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kY]) > 0.0f)
        {
            tmpy = tmp_pos[kY];
            tmp_pos[kY] += max_apos[kY];
            if ( EMPTY_BIT_FIELD != pchr->test_wall( tmp_pos ) )
            {
                // restore the old values
                tmp_pos[kY] = tmpy;
            }
            else
            {
                //pchr->vel[kY] += pchr->phys.apos_coll[kY] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kZ]) > 0.0f)
        {
            tmp_pos[kZ] += max_apos[kZ];
            if ( tmp_pos[kZ] < pchr->getObjectPhysics().getGroundElevation() )
            {
                // restore the old values
                tmp_pos[kZ] = pchr->getObjectPhysics().getGroundElevation();
                if ( pchr->vel[kZ] < 0 )
                {
                    pchr->vel[kZ] += -( 1.0f + pchr->getProfile()->getBounciness() ) * pchr->vel[kZ];
                }
                position_updated = true;
            }
            else
            {
                //pchr->vel[kZ] += pchr->phys.apos_coll[kZ] * bump_str;
                position_updated = true;
            }
        }

        if ( position_updated )
        {
            pchr->setPosition(tmp_pos);
        }
    }

    // accumulate the accumulators
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        float tmpx, tmpy;
        bool position_updated = false;
        Vector3f max_apos;

        if(particle->isTerminated()) {
            continue;
        }

        Vector3f tmp_pos = particle->getPosition();

        // do the "integration" of the accumulated accelerations
        particle->vel += particle->phys.avel;

        position_updated = false;

        // get a net displacement vector from aplat and acoll
        {
            // create a temporary apos_t
            apos_t  apos_tmp;

            // copy 1/2 of the data over
            apos_tmp = particle->phys.aplat;

            // get the resultant apos_t
            apos_tmp.join(particle->phys.acoll);

            // turn this into a vector
            apos_t::evaluate(apos_tmp, max_apos);
        }

        max_apos[kX] = Ego::Math::constrain( max_apos[kX], -Info<float>::Grid::Size(), Info<float>::Grid::Size());
        max_apos[kY] = Ego::Math::constrain( max_apos[kY], -Info<float>::Grid::Size(), Info<float>::Grid::Size());
        max_apos[kZ] = Ego::Math::constrain( max_apos[kZ], -Info<float>::Grid::Size(), Info<float>::Grid::Size());

        // do the "integration" on the position
        if (std::abs(max_apos[kX]) > 0.0f)
        {
            tmpx = tmp_pos[kX];
            tmp_pos[kX] += max_apos[kX];
            if ( EMPTY_BIT_FIELD != particle->test_wall( tmp_pos ) )
            {
                // restore the old values
                tmp_pos[kX] = tmpx;
            }
            else
            {
                //bdl.prt_ptr->vel[kX] += bdl.prt_ptr->phys.apos_coll[kX] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kY]) > 0.0f)
        {
            tmpy = tmp_pos[kY];
            tmp_pos[kY] += max_apos[kY];
            if ( EMPTY_BIT_FIELD != particle->test_wall( tmp_pos ) )
            {
                // restore the old values
                tmp_pos[kY] = tmpy;
            }
            else
            {
                //bdl.prt_ptr->vel[kY] += bdl.prt_ptr->phys.apos_coll[kY] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kZ]) > 0.0f)
        {
            tmp_pos[kZ] += max_apos[kZ];
            if ( tmp_pos[kZ] < particle->enviro.floor_level )
            {
                // restore the old values
                tmp_pos[kZ] = particle->enviro.floor_level;
                if ( particle->vel[kZ] < 0 )
                {
                    particle->vel[kZ] += -( 1.0f + particle->getProfile()->dampen ) * particle->vel[kZ];
                }
                position_updated = true;
            }
            else
            {
                //bdl.prt_ptr->vel[kZ] += bdl.prt_ptr->phys.apos_coll[kZ] * bump_str;
                position_updated = true;
            }
        }

        // Change the direction of the particle
        if ( particle->getProfile()->rotatetoface )
        {
            // Turn to face new direction
            particle->facing = Facing(vec_to_facing( particle->vel[kX] , particle->vel[kY] ));
        }

        if ( position_updated )
        {
            particle->setPosition(tmp_pos);
        }
    }
}

void CollisionSystem::updateObjectCollisions()
{
    std::unordered_set<std::shared_ptr<Object>> handledObjects;

    //Detect character -> character collisions
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator()) {

        //Can we collide?
        if (!object->canCollide()) {
            continue;
        }
        handledObjects.insert(object);

        //First check if this object is still attached to it's Platform
        const std::shared_ptr<Object> &platform = _currentModule->getObjectHandler()[object->onwhichplatform_ref];
        if(platform)
        {
            //If we are no longer colliding in the horizontal plane, then we are disconnected
            if(!object->getAABB2D().overlaps(platform->getAABB2D()))
            {
                object->getObjectPhysics().detachFromPlatform();
            }
        }

        //TODO: Remove this messy block and replace it with something better
        // use the object velocity to figure out where the volume that the object will occupy during this update
        // convert the oct_bb_t to a correct BSP_aabb_t
        oct_bb_t tmp_oct;
        phys_expand_chr_bb(object.get(), 0.0f, 1.0f, tmp_oct);
        const AABB2f aabb2d = AABB2f(Vector2f(tmp_oct._mins[OCT_X], tmp_oct._mins[OCT_Y]), Vector2f(tmp_oct._maxs[OCT_X], tmp_oct._maxs[OCT_Y]));

        //Do not collide scenery with other scenery objects - unless they can use platforms,
        //for example boxes stacked on top of other boxes
        bool canCollideWithScenery = !object->isScenery() || object->canuseplatforms;

        // Check collisions to nearby Objects
        std::vector<std::shared_ptr<Object>> possibleCollisions;
        _currentModule->getObjectHandler().findObjects(aabb2d, possibleCollisions, canCollideWithScenery);
        for (const std::shared_ptr<Object> &other : possibleCollisions)
        {
            //Skip possible interactions that have already been handled earlier this iteration
            if(handledObjects.find(other) != handledObjects.end()) {
                continue;
            }

            //Can it collide?
            if(!other->canCollide()) {
                continue;
            }

            //Detect any collisions and handle it if needed
            float tmin, tmax;
            if(detectCollision(object, other, &tmin, &tmax)) {
                handleCollision(object, other, tmin, tmax);
            }
        }
    }
}

void CollisionSystem::updateParticleCollisions()
{
    //Check collisions with particles
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(!particle->canCollide()) {
            continue;
        }

        //First check if this Particle is still attached to a platform
        if (particle->onwhichplatform_update < update_wld && _currentModule->getObjectHandler().exists(particle->onwhichplatform_ref)) {
            particle->getParticlePhysics().detachFromPlatform();
        }

        // use the object velocity to figure out where the volume that the object will occupy during this update
        // convert the oct_bb_t to a correct AABB2f
        oct_bb_t   tmp_oct;
        phys_expand_prt_bb(particle.get(), 0.0f, 1.0f, tmp_oct);
        const AABB2f aabb2d = AABB2f(Vector2f(tmp_oct._mins[OCT_X], tmp_oct._mins[OCT_Y]), Vector2f(tmp_oct._maxs[OCT_X], tmp_oct._maxs[OCT_Y]));

        //Detect collisions with nearby Objects
        std::vector<std::shared_ptr<Object>> possibleCollisions;
         _currentModule->getObjectHandler().findObjects(aabb2d, possibleCollisions, true);
        for (const std::shared_ptr<Object> &object : possibleCollisions)
        {
            //Is it a valid collision?
            if(!object->canCollide()) {
                continue;
            }

            //Detect any collisions and handle it if needed
            float tmin, tmax;
            if(detectCollision(particle, object, &tmin, &tmax)) {
                do_prt_platform_detection(object->getObjRef(), particle->getParticleID());
                do_chr_prt_collision(object, particle, tmin, tmax);
            }
        }
    }    
}

bool CollisionSystem::detectCollision(const std::shared_ptr<Ego::Particle> &particle, const std::shared_ptr<Object> &object, float *tmin, float *tmax) const
{
    // particles don't "collide" with anything they are attached to.
    // that only happes through doing bump particle damage
    if (particle->getAttachedObject() == object)
    {
        return false;
    }

    //Detect collisions with platforms?
    BIT_FIELD testPlatform = EMPTY_BIT_FIELD;
    if ( object->platform /*&& ( SPRITE_SOLID == particle->type )*/ ) {
        SET_BIT(testPlatform, PHYS_PLATFORM_OBJ1);
    }

    // Some information about the estimated collision.
    //TODO: ZF> hmmm unused?
    oct_bb_t cv;

    // detect a when the possible collision occurred
    return phys_intersect_oct_bb(object->chr_min_cv, object->getPosition(), object->vel, particle->prt_max_cv, particle->getPosition(), particle->vel, testPlatform, cv, tmin, tmax);
}

bool CollisionSystem::detectCollision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB, float *tmin, float *tmax) const
{
    // "non-interacting" objects interact with platforms
    if ((0 == objectA->bump.size && !objectB->platform ) ||
        (0 == objectB->bump.size && !objectA->platform )) {
        return false;
    }

    // handle the dismount exception
    if (objectA->dismount_timer > 0 && objectA->dismount_object == objectB->getObjRef()) {
        return false;
    }
    if (objectB->dismount_timer > 0 && objectB->dismount_object == objectA->getObjRef()) {
        return false;
    }

    //Is it a platform collision?
    BIT_FIELD testPlatform = EMPTY_BIT_FIELD;
    if (objectA->platform && objectB->canuseplatforms) {
        SET_BIT(testPlatform, PHYS_PLATFORM_OBJ1);
    }
    if (objectB->platform && objectA->canuseplatforms) {
        SET_BIT(testPlatform, PHYS_PLATFORM_OBJ2);
    }

    // Some information about the estimated collision.
    //TODO: ZF> hmmm unused?
    oct_bb_t cv;

    // detect a when the possible collision occurred
    return phys_intersect_oct_bb(objectA->chr_max_cv, objectA->getPosition(), objectA->vel, objectB->chr_max_cv, objectB->getPosition(), objectB->vel, testPlatform, cv, tmin, tmax);
}

void CollisionSystem::handleCollision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB, const float tmin, const float tmax)
{
    //Try to mount A with B
    if(objectA->canMount(objectB)) {
        if(handleMountingCollision(objectA, objectB)) {
            return;
        }
    }

    //Try to mount B with A
    if(objectB->canMount(objectA)) {
        if(handleMountingCollision(objectB, objectA)) {
            return;
        }
    }

    //Try to resolve any platform collision
    //This attaches objects to other platform objects
    handlePlatformCollision(objectA, objectB);

    //TODO: inline?
    do_chr_chr_collision(objectA, objectB, tmin, tmax);
}

bool CollisionSystem::handleMountingCollision(const std::shared_ptr<Object> &character, const std::shared_ptr<Object> &mount)
{
    //Do some collision checks
	bool collideXY = (Vector2f(character->getPosX(), character->getPosY()) - Vector2f(mount->getPosX(), mount->getPosY())).length() < MOUNTTOLERANCE;

	bool collideZ = (mount->getPosZ() + mount->chr_min_cv._maxs[OCT_Z]) < character->getPosZ();

    //If we are falling on top of the mount, then we are trying to mount
	bool characterWantsToMount = collideXY && collideZ;

    //If we are facing the mount and jumping towards it, then we are trying to mount
    //else if(collideXY) {
    //    if(character->jump_timer > 0 && character->isFacingLocation(mount->getPosX(), mount->getPosY())) {
    //        characterWantsToMount = true;
    //    }
    //}

    //Attempt to mount?
    if(characterWantsToMount) {
        return character->getObjectPhysics().attachToObject(mount, GRIP_ONLY);
    }

    return false;
}

bool CollisionSystem::handlePlatformCollision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB)
{
    oct_vec_v2_t odepth;

    const auto ichr_a = objectA->getObjRef();
    const auto ichr_b = objectB->getObjRef();

    // only check possible object-platform interactions
    bool platform_a = objectB->canuseplatforms && !_currentModule->getObjectHandler().exists(objectB->onwhichplatform_ref) && objectA->platform;
    bool platform_b = objectA->canuseplatforms && !_currentModule->getObjectHandler().exists(objectA->onwhichplatform_ref) && objectB->platform;

    //Only allow scenery objects on top of other scenery objects
    if(objectA->isScenery() != objectB->isScenery()) {
        platform_a &= objectA->isScenery();
        platform_b &= objectB->isScenery();
    }

    if ( !platform_a && !platform_b ) return false;

    odepth[OCT_Z] = std::min(objectB->chr_min_cv._maxs[OCT_Z] + objectB->getPosZ(), objectA->chr_min_cv._maxs[OCT_Z] + objectA->getPosZ()) -
                    std::max(objectB->chr_min_cv._mins[OCT_Z] + objectB->getPosZ(), objectA->chr_min_cv._mins[OCT_Z] + objectA->getPosZ() );

    bool collide_z  = odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE;

    if ( !collide_z ) return false;

    // determine how the characters can be attached
    bool chara_on_top = true;
    odepth[OCT_Z] = 2 * PLATTOLERANCE;
    if ( platform_a && platform_b )
    {
        float depth_a, depth_b;

        depth_a = ( objectB->getPosZ() + objectB->chr_min_cv._maxs[OCT_Z] ) - ( objectA->getPosZ() + objectA->chr_min_cv._mins[OCT_Z] );
        depth_b = ( objectA->getPosZ() + objectA->chr_min_cv._maxs[OCT_Z] ) - ( objectB->getPosZ() + objectB->chr_min_cv._mins[OCT_Z] );

        odepth[OCT_Z] = std::min( objectB->getPosZ() + objectB->chr_min_cv._maxs[OCT_Z], objectA->getPosZ() + objectA->chr_min_cv._maxs[OCT_Z] ) -
                        std::max( objectB->getPosZ() + objectB->chr_min_cv._mins[OCT_Z], objectA->getPosZ() + objectA->chr_min_cv._mins[OCT_Z] );

        chara_on_top = std::abs(odepth[OCT_Z] - depth_a) < std::abs(odepth[OCT_Z] - depth_b);

        // the collision is determined by the platform size
        if ( chara_on_top )
        {
            // size of a doesn't matter
            odepth[OCT_X]  = std::min(( objectB->chr_min_cv._maxs[OCT_X] + objectB->getPosX() ) - objectA->getPosX(),
                                        objectA->getPosX() - ( objectB->chr_min_cv._mins[OCT_X] + objectB->getPosX() ) );

            odepth[OCT_Y]  = std::min(( objectB->chr_min_cv._maxs[OCT_Y] + objectB->getPosY() ) -  objectA->getPosY(),
                                        objectA->getPosY() - ( objectB->chr_min_cv._mins[OCT_Y] + objectB->getPosY() ) );

            odepth[OCT_XY] = std::min(( objectB->chr_min_cv._maxs[OCT_XY] + ( objectB->getPosX() + objectB->getPosY() ) ) - ( objectA->getPosX() + objectA->getPosY() ),
                                      ( objectA->getPosX() + objectA->getPosY() ) - ( objectB->chr_min_cv._mins[OCT_XY] + ( objectB->getPosX() + objectB->getPosY() ) ) );

            odepth[OCT_YX] = std::min(( objectB->chr_min_cv._maxs[OCT_YX] + ( -objectB->getPosX() + objectB->getPosY() ) ) - ( -objectA->getPosX() + objectA->getPosY() ),
                                      ( -objectA->getPosX() + objectA->getPosY() ) - ( objectB->chr_min_cv._mins[OCT_YX] + ( -objectB->getPosX() + objectB->getPosY() ) ) );
        }
        else
        {
            // size of b doesn't matter

            odepth[OCT_X]  = std::min(( objectA->chr_min_cv._maxs[OCT_X] + objectA->getPosX() ) - objectB->getPosX(),
                                        objectB->getPosX() - ( objectA->chr_min_cv._mins[OCT_X] + objectA->getPosX() ) );

            odepth[OCT_Y]  = std::min(( objectA->chr_min_cv._maxs[OCT_Y] + objectA->getPosY() ) -  objectB->getPosY(),
                                        objectB->getPosY() - ( objectA->chr_min_cv._mins[OCT_Y] + objectA->getPosY() ) );

            odepth[OCT_XY] = std::min(( objectA->chr_min_cv._maxs[OCT_XY] + ( objectA->getPosX() + objectA->getPosY() ) ) - ( objectB->getPosX() + objectB->getPosY() ),
                                      ( objectB->getPosX() + objectB->getPosY() ) - ( objectA->chr_min_cv._mins[OCT_XY] + ( objectA->getPosX() + objectA->getPosY() ) ) );

            odepth[OCT_YX] = std::min(( objectA->chr_min_cv._maxs[OCT_YX] + ( -objectA->getPosX() + objectA->getPosY() ) ) - ( -objectB->getPosX() + objectB->getPosY() ),
                                      ( -objectB->getPosX() + objectB->getPosY() ) - ( objectA->chr_min_cv._mins[OCT_YX] + ( -objectA->getPosX() + objectA->getPosY() ) ) );
        }
    }
    else if ( platform_a )
    {
        chara_on_top = false;
        odepth[OCT_Z] = ( objectA->getPosZ() + objectA->chr_min_cv._maxs[OCT_Z] ) - ( objectB->getPosZ() + objectB->chr_min_cv._mins[OCT_Z] );

        // size of b doesn't matter

        odepth[OCT_X] = std::min((objectA->chr_min_cv._maxs[OCT_X] + objectA->getPosX() ) - objectB->getPosX(),
                                  objectB->getPosX() - ( objectA->chr_min_cv._mins[OCT_X] + objectA->getPosX() ) );

        odepth[OCT_Y] = std::min((objectA->chr_min_cv._maxs[OCT_Y] + objectA->getPosY()) - objectB->getPosY(),
                                  objectB->getPosY() - ( objectA->chr_min_cv._mins[OCT_Y] + objectA->getPosY() ) );

        odepth[OCT_XY] = std::min((objectA->chr_min_cv._maxs[OCT_XY] + (objectA->getPosX() + objectA->getPosY())) - (objectB->getPosX() + objectB->getPosY()),
                                  ( objectB->getPosX() + objectB->getPosY() ) - ( objectA->chr_min_cv._mins[OCT_XY] + ( objectA->getPosX() + objectA->getPosY() ) ) );

        odepth[OCT_YX] = std::min((objectA->chr_min_cv._maxs[OCT_YX] + (-objectA->getPosX() + objectA->getPosY())) - (-objectB->getPosX() + objectB->getPosY()),
                                  ( -objectB->getPosX() + objectB->getPosY() ) - ( objectA->chr_min_cv._mins[OCT_YX] + ( -objectA->getPosX() + objectA->getPosY() ) ) );
    }
    else if ( platform_b )
    {
        chara_on_top = true;
        odepth[OCT_Z] = ( objectB->getPosZ() + objectB->chr_min_cv._maxs[OCT_Z] ) - ( objectA->getPosZ() + objectA->chr_min_cv._mins[OCT_Z] );

        // size of a doesn't matter
        odepth[OCT_X] = std::min((objectB->chr_min_cv._maxs[OCT_X] + objectB->getPosX()) - objectA->getPosX(),
                                  objectA->getPosX() - ( objectB->chr_min_cv._mins[OCT_X] + objectB->getPosX() ) );

        odepth[OCT_Y] = std::min(objectB->chr_min_cv._maxs[OCT_Y] + (objectB->getPosY() - objectA->getPosY()),
                                 ( objectA->getPosY() - objectB->chr_min_cv._mins[OCT_Y] ) + objectB->getPosY() );

        odepth[OCT_XY] = std::min((objectB->chr_min_cv._maxs[OCT_XY] + (objectB->getPosX() + objectB->getPosY())) - (objectA->getPosX() + objectA->getPosY()),
                                  ( objectA->getPosX() + objectA->getPosY() ) - ( objectB->chr_min_cv._mins[OCT_XY] + ( objectB->getPosX() + objectB->getPosY() ) ) );

        odepth[OCT_YX] = std::min(( objectB->chr_min_cv._maxs[OCT_YX] + ( -objectB->getPosX() + objectB->getPosY() ) ) - ( -objectA->getPosX() + objectA->getPosY() ),
                                  ( -objectA->getPosX() + objectA->getPosY() ) - ( objectB->chr_min_cv._mins[OCT_YX] + ( -objectB->getPosX() + objectB->getPosY() ) ) );

    }

    bool collide_x  = odepth[OCT_X]  > 0.0f;
    bool collide_y  = odepth[OCT_Y]  > 0.0f;
    bool collide_xy = odepth[OCT_XY] > 0.0f;
    bool collide_yx = odepth[OCT_YX] > 0.0f;
    collide_z  = odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE;

    if ( collide_x && collide_y && collide_xy && collide_yx && collide_z )
    {
        // check for the best possible attachment
        if ( chara_on_top )
        {
            if ( objectB->getPosZ() + objectB->chr_min_cv._maxs[OCT_Z] > objectA->targetplatform_level )
            {
                objectA->targetplatform_level = objectB->getPosZ() + objectB->chr_min_cv._maxs[OCT_Z];
                objectA->targetplatform_ref   = ichr_b;

                return objectA->getObjectPhysics().attachToPlatform(objectB);
            }
        }
        else
        {
            if ( objectA->getPosZ() + objectA->chr_min_cv._maxs[OCT_Z] > objectB->targetplatform_level )
            {
                objectB->targetplatform_level = objectA->getPosZ() + objectA->chr_min_cv._maxs[OCT_Z];
                objectB->targetplatform_ref   = ichr_a;

                return objectB->getObjectPhysics().attachToPlatform(objectA);
            }
        }
    }

    return false;
}

bool do_chr_chr_collision(const std::shared_ptr<Object> &objectA, const std::shared_ptr<Object> &objectB, const float tmin, const float tmax)
{
    const ObjectRef ichr_a = objectA->getObjRef();
    const ObjectRef ichr_b = objectB->getObjRef();

    // platform interaction. if the onwhichplatform_ref is set, then
    // all collision tests have been met
    if ( ichr_a == objectB->onwhichplatform_ref )
    {
        // this is handled in ObjectPhysics.cpp
        return true;
    }

    // platform interaction. if the onwhichplatform_ref is set, then
    // all collision tests have been met
    if ( ichr_b == objectA->onwhichplatform_ref )
    {
        // this is handled in ObjectPhysics.cpp
        return true;
    }

    // items can interact with platforms but not with other characters/objects
    if ( (objectA->isItem() && !objectA->platform) || (objectB->isItem() && !objectB->platform) ) {
        return false;
    }

    // don't interact with your mount, or your held items
    if (ichr_a == objectB->attachedto || ichr_b == objectA->attachedto) {
        return false;
    }

    // don't do anything if there is no interaction strength
    if (0.0f == objectA->bump_stt.size || 0.0f == objectB->bump_stt.size) {
        return false;
    }

    float interaction_strength = 0.1f + (0.9f-objectA->phys.bumpdampen) * (0.9f-objectB->phys.bumpdampen);
    
    //ZF> This was supposed to make ghosts more insubstantial, but it also affects invisible characters
    //interaction_strength *= objectA->inst.alpha * INV_FF;
    //interaction_strength *= objectB->inst.alpha * INV_FF;

    // reduce your interaction strength if you have just detached from an object
    if ( objectA->dismount_object == ichr_b )
    {
        float dismount_lerp = ( float )objectA->dismount_timer / static_cast<float>(Object::PHYS_DISMOUNT_TIME);
        dismount_lerp = Ego::Math::constrain( dismount_lerp, 0.0f, 1.0f );

        interaction_strength *= dismount_lerp;
    }

    if ( objectB->dismount_object == ichr_a )
    {
        float dismount_lerp = ( float )objectB->dismount_timer / static_cast<float>(Object::PHYS_DISMOUNT_TIME);
        dismount_lerp = Ego::Math::constrain( dismount_lerp, 0.0f, 1.0f );

        interaction_strength *= dismount_lerp;
    }

    // seriously reduce the interaction_strength with mounts
    // this thould allow characters to mount certain mounts a lot easier
    if (( objectA->isMount() && ObjectRef::Invalid == objectA->holdingwhich[SLOT_LEFT] && !objectB->isMount() ) ||
        ( objectB->isMount() && ObjectRef::Invalid == objectB->holdingwhich[SLOT_LEFT] && !objectA->isMount() ) )
    {
        interaction_strength *= 0.75f;
    }

    // reduce the interaction strength with platforms
    // that are overlapping with the platform you are actually on
    if ( objectB->canuseplatforms && objectA->platform && ObjectRef::Invalid != objectB->onwhichplatform_ref && ichr_a != objectB->onwhichplatform_ref )
    {
        float lerp_z = ( objectB->getPosZ() - ( objectA->getPosZ() + objectA->chr_min_cv._maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = Ego::Math::constrain(lerp_z, -1.0f, 1.0f);

        if ( lerp_z >= 0.0f )
        {
            interaction_strength = 0.0f;
        }
        else
        {
            interaction_strength *= -lerp_z;
        }
    }

    if ( objectA->canuseplatforms && objectB->platform && ObjectRef::Invalid != objectA->onwhichplatform_ref && ichr_b != objectA->onwhichplatform_ref )
    {
        float lerp_z = ( objectA->getPosZ() - ( objectB->getPosZ() + objectB->chr_min_cv._maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = Ego::Math::constrain( lerp_z, -1.0f, +1.0f );

        if ( lerp_z >= 0.0f )
        {
            interaction_strength = 0.0f;
        }
        else
        {
            interaction_strength *= -lerp_z;
        }
    }

	// object bounding boxes shifted so that they are in the correct place on the map
	oct_bb_t map_bb_a, map_bb_b;

    // shift the character bounding boxes to be centered on their positions
    oct_bb_t::translate(objectA->chr_min_cv, objectA->getPosition(), map_bb_a);
    oct_bb_t::translate(objectB->chr_min_cv, objectB->getPosition(), map_bb_b);

    // make the object more like a table if there is a platform-like interaction
    float exponent = 1.0f;
    if ( objectA->canuseplatforms && objectB->platform ) exponent += 2;
    if ( objectB->canuseplatforms && objectA->platform ) exponent += 2;

	float recoil_a, recoil_b;



	Vector3f nrm;
	oct_vec_v2_t odepth;
	bool bump = false;

    // use the info from the collision volume to determine whether the objects are colliding
    bool collision = tmin > 0.0f;

    // estimate the collision normal at the point of contact
    bool valid_normal = false;
    float depth_min    = 0.0f;
    if ( collision )
    {
        // find the collision volumes at 10% overlap
        oct_bb_t exp1, exp2;

        float tmp_min = tmin;
        float tmp_max = tmin + ( tmax - tmin ) * 0.1f;

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb(map_bb_a, objectA->vel, tmp_min, tmp_max, exp1);
        phys_expand_oct_bb(map_bb_b, objectB->vel, tmp_min, tmp_max, exp2);

        valid_normal = phys_estimate_collision_normal(exp1, exp2, exponent, odepth, nrm, depth_min);
    }

    if ( !collision || depth_min <= 0.0f )
    {
        valid_normal = phys_estimate_pressure_normal(map_bb_a, map_bb_b, exponent, odepth, nrm, depth_min);
    }

    if ( depth_min <= 0.0f )
        return false;

    // if we can't obtain a valid collision normal, we fail
    if ( !valid_normal ) return false;

    //------------------
    // do character-character interactions

    // calculate a "mass" for each object, taking into account possible infinite masses
    float wta = objectA->getObjectPhysics().getMass();
    float wtb = objectB->getObjectPhysics().getMass();

    // make a special exception for interaction between "Mario platforms"
    if (( wta < 0.0f && objectA->platform ) && ( wtb < 0.0f && objectA->platform ) )
    {
        return false;
    }

    // make a special exception for immovable scenery objects
    // they can collide, but cannot push each other apart... that might mess up the scenery ;)
    if ( !collision && objectA->isScenery() && objectB->isScenery() )
    {
        return false;
    }

    // determine the relative effect of impulses, given the known weights
    get_recoil_factors( wta, wtb, &recoil_a, &recoil_b );

    //---- calculate the character-character interactions
    {
        const float max_pressure_strength = 0.25f;//1.0f - std::min(objectA->phys.dampen, objectB->phys.dampen);
        float pressure_strength     = max_pressure_strength * interaction_strength;

        Vector3f pdiff_a;

        bool need_displacement = false;
        bool need_velocity = false;

        Vector3f vdiff_a;

        if ( depth_min <= 0.0f || collision )
        {
            need_displacement = false;
            pdiff_a = Vector3f::zero();
        }
        else
        {
            // add a small amount to the pressure difference so that
            // the function will actually separate the objects in a finite number
            // of iterations
            need_displacement = (recoil_a > 0.0f) || (recoil_b > 0.0f);
            pdiff_a = nrm * (depth_min + 1.0f);
        }

        // find the relative velocity
        vdiff_a = objectB->vel - objectA->vel;

        need_velocity = false;
        if (vdiff_a.length_abs() > 1e-6)
        {
            need_velocity = (recoil_a > 0.0f) || (recoil_b > 0.0f);
        }

        //---- handle the relative velocity
        if ( need_velocity )
        {

            // what type of interaction is this? (collision or pressure)
            if ( collision )
            {
                // !!!! COLLISION !!!!

                // an actual bump, use impulse to make the objects bounce appart

                Vector3f vdiff_para_a, vdiff_perp_a;

                // generic coefficient of restitution.
                float cr = objectA->phys.dampen * objectB->phys.dampen;

                // decompose this relative to the collision normal
                fvec3_decompose(vdiff_a, nrm, vdiff_perp_a, vdiff_para_a);

                if (recoil_a > 0.0f)
                {
                    Vector3f vimp_a = vdiff_perp_a * +(recoil_a * (1.0f + cr) * interaction_strength);
                    objectA->phys.sum_avel(vimp_a);
                }

                if (recoil_b > 0.0f)
                {
                    Vector3f vimp_b = vdiff_perp_a * -(recoil_b * (1.0f + cr) * interaction_strength);
                    objectB->phys.sum_avel(vimp_b);
                }

                // this was definitely a bump
                bump = true;
            }
            else
            {
                // !!!! PRESSURE !!!!

                // not a bump at all. two objects are rubbing against one another
                // and continually overlapping.
                //
                // reduce the relative velocity if the objects are moving towards each other,
                // but ignore it if they are moving away.

                // use pressure to push them appart. reduce their relative velocities.

                float distance = (objectA->getPosition() - objectB->getPosition()).length();
                distance /= std::max(objectA->bump.size, objectB->bump.size);
                if(distance > 0.0f)
                {
                    objectA->phys.sum_avel(nrm * distance * recoil_a * interaction_strength);
                    objectB->phys.sum_avel(-nrm * distance * recoil_b * interaction_strength);

                    // you could "bump" something if you changed your velocity, even if you were still touching
                    bump = ((objectA->vel.dot(nrm) * objectA->vel_old.dot(nrm)) < 0) ||
                           ((objectB->vel.dot(nrm) * objectB->vel_old.dot(nrm)) < 0);   
                }
            }

        }

        //---- fix the displacement regardless of what kind of interaction
        if ( need_displacement )
        {
            if ( recoil_a > 0.0f )
            {
                Vector3f pimp_a = pdiff_a * +(recoil_a * pressure_strength);
                objectA->phys.sum_acoll(pimp_a);
            }

            if ( recoil_b > 0.0f )
            {
                Vector3f pimp_b = pdiff_a * -(recoil_b * pressure_strength);
                objectB->phys.sum_acoll(pimp_b);
            }
        }
    }

    if ( bump )
    {
        ai_state_t::set_bumplast(objectA->ai, ichr_b);
        ai_state_t::set_bumplast(objectB->ai, ichr_a);

        //Destroy stealth for both objects if they are not friendly
        if(!objectA->isScenery() && !objectB->isScenery() && objectA->getTeam().hatesTeam(objectB->getTeam())) {
            if(!objectA->hasPerk(Ego::Perks::SHADE)) objectA->deactivateStealth();
            if(!objectA->hasPerk(Ego::Perks::SHADE)) objectB->deactivateStealth();
        }
    }

    return true;
}

static void get_recoil_factors( float wta, float wtb, float * recoil_a, float * recoil_b )
{
    float loc_recoil_a, loc_recoil_b;

    if ( NULL == recoil_a ) recoil_a = &loc_recoil_a;
    if ( NULL == recoil_b ) recoil_b = &loc_recoil_b;

    if ( wta >= Ego::Physics::CHR_INFINITE_WEIGHT ) wta = -static_cast<float>(Ego::Physics::CHR_INFINITE_WEIGHT);
    if ( wtb >= Ego::Physics::CHR_INFINITE_WEIGHT ) wtb = -static_cast<float>(Ego::Physics::CHR_INFINITE_WEIGHT);

    if ( wta < 0.0f && wtb < 0.0f )
    {
        *recoil_a = 0.5f;
        *recoil_b = 0.5f;
    }
    else if ( wta == wtb )
    {
        *recoil_a = 0.5f;
        *recoil_b = 0.5f;
    }
    else if ( wta < 0.0f || 0.0f == wtb )
    {
        *recoil_a = 0.0f;
        *recoil_b = 1.0f;
    }
    else if ( wtb < 0.0f || 0.0f == wta )
    {
        *recoil_a = 1.0f;
        *recoil_b = 0.0f;
    }
    else
    {
        *recoil_a = wtb / ( wta + wtb );
        *recoil_b = wta / ( wta + wtb );
    }
}

} //namespace Physics
} //namespace Ego
