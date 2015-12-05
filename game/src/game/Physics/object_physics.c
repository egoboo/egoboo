#include "ObjectPhysics.h"
#include "game/game.h"
#include "game/player.h"
#include "game/renderer_2d.h"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/Physics/PhysicalConstants.hpp"
#include "game/Core/GameEngine.hpp"

#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct grab_data_t
{
    grab_data_t() :
        object(nullptr),
        horizontalDistance(0.0f),
        verticalDistance(0.0f),
        visible(true),
        isFacingObject(false)
    {
        //ctor
    }

    std::shared_ptr<Object> object;
    float horizontalDistance;
    float verticalDistance;
    bool visible;
    bool isFacingObject;
};

void move_one_character_get_environment( Object * pchr )
{
    if (!pchr || pchr->isTerminated()) return;
    chr_environment_t& enviro = pchr->enviro;

    // determine if the character is standing on a platform
    Object *pplatform = nullptr;
    if ( _currentModule->getObjectHandler().exists( pchr->onwhichplatform_ref ) )
    {
        pplatform = _currentModule->getObjectHandler().get( pchr->onwhichplatform_ref );
    }

    ego_mesh_t *mesh = _currentModule->getMeshPointer().get();

    //---- character "floor" level
    float grid_level = mesh->getElevation(Vector2f(pchr->getPosX(), pchr->getPosY()), false);
    float water_level = mesh->getElevation(Vector2f(pchr->getPosX(), pchr->getPosY()), true);

    // chr_set_enviro_grid_level() sets up the reflection level and reflection matrix
    if (grid_level != pchr->enviro.grid_level) {
        pchr->enviro.grid_level = grid_level;

        chr_instance_t::apply_reflection_matrix(pchr->inst, grid_level);
    }

    enviro.grid_lerp  = ( pchr->getPosZ() - grid_level ) / PLATTOLERANCE;
    enviro.grid_lerp  = Ego::Math::constrain( enviro.grid_lerp, 0.0f, 1.0f );

    enviro.water_level = water_level;
    enviro.water_lerp  = ( pchr->getPosZ() - water_level ) / PLATTOLERANCE;
    enviro.water_lerp  = Ego::Math::constrain( enviro.water_lerp, 0.0f, 1.0f );

    // prime the environment
    enviro.ice_friction = Ego::Physics::g_environment.icefriction;

    // The actual level of the floor underneath the character.
    if (pplatform)
    {
        enviro.floor_level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }
    else
    {
        enviro.floor_level = pchr->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 ? water_level : grid_level;
    }

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    enviro.level = enviro.floor_level;
    if (pplatform)
    {
        enviro.level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }

    //---- The flying height of the character, the maximum of tile level, platform level and water level
    if ( 0 != mesh->test_fx( pchr->getTile(), MAPFX_WATER ) )
    {
        enviro.fly_level = std::max(enviro.level, water._surface_level);
    }

    if ( enviro.fly_level < 0 )
    {
        enviro.fly_level = 0;  // fly above pits...
    }

    // set the zlerp
    enviro.zlerp = (pchr->getPosZ() - enviro.level) / PLATTOLERANCE;
    enviro.zlerp = Ego::Math::constrain(enviro.zlerp, 0.0f, 1.0f);

    enviro.grounded = !pchr->isFlying() && enviro.zlerp <= 0.25f;

    //---- the "twist" of the floor
    enviro.grid_twist = mesh->get_twist(pchr->getTile());

    // the "watery-ness" of whatever water might be here
    enviro.is_watery = water._is_water && enviro.inwater;
    enviro.is_slippy = !enviro.is_watery && ( 0 != mesh->test_fx( pchr->getTile(), MAPFX_SLIPPY ) );

    //---- the friction of the fluid we are in
    if (enviro.is_watery)
    {
        //Athletics perk halves penality for moving in water
        if(pchr->hasPerk(Ego::Perks::ATHLETICS)) {
            enviro.fluid_friction_vrt  = (Ego::Physics::g_environment.waterfriction + Ego::Physics::g_environment.airfriction)*0.5f;
            enviro.fluid_friction_hrz  = (Ego::Physics::g_environment.waterfriction + Ego::Physics::g_environment.airfriction)*0.5f;
        }
        else {
            enviro.fluid_friction_vrt = Ego::Physics::g_environment.waterfriction;
            enviro.fluid_friction_hrz = Ego::Physics::g_environment.waterfriction;            
        }
    }
    else if (pplatform)
    {
        enviro.fluid_friction_hrz = 1.0f;
        enviro.fluid_friction_vrt = 1.0f;
    }
    else
    {
        // like real-life air friction
        enviro.fluid_friction_hrz = Ego::Physics::g_environment.airfriction;
        enviro.fluid_friction_vrt = Ego::Physics::g_environment.airfriction;            
    }

    //---- friction
    if (pchr->isFlying())
    {
        if ( pchr->platform )
        {
            // override the z friction for platforms.
            // friction in the z direction will make the bouncing stop
            enviro.fluid_friction_vrt = 1.0f;
        }
        enviro.friction_hrz = 1.0f;
    }
    else
    {
        enviro.friction_hrz = enviro.zlerp * 1.0f + (1.0f - enviro.zlerp) * Ego::Physics::g_environment.noslipfriction;
    }

    //---- jump stuff
    if ( pchr->isFlying() )
    {
        // Flying
        pchr->jumpready = false;
    }
    else
    {
        // Character is in the air
        pchr->jumpready = enviro.grounded;

        // Down jump timer
        if ((pchr->isBeingHeld() || pchr->jumpready || pchr->jumpnumber > 0) && pchr->jump_timer > 0) { 
            pchr->jump_timer--;
        }

        // Do ground hits
        if ( enviro.grounded && pchr->vel[kZ] < -Ego::Physics::STOP_BOUNCING && pchr->hitready )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_HITGROUND );
            pchr->hitready = false;
        }

        if ( enviro.grounded && 0 == pchr->jump_timer )
        {
            // Reset jumping on flat areas of slippiness
            if(!enviro.is_slippy || g_meshLookupTables.twist_flat[enviro.grid_twist]) {
                pchr->jumpnumber = pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);                
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool character_grab_stuff( ObjectRef ichr_a, grip_offset_t grip_off, bool grab_people )
{
    /// @author ZZ
    /// @details This function makes the character pick up an item if there's one around

    const auto color_red = Ego::Math::Colour4f::parse(0xFF, 0x7F, 0x7F, 0xFF);
    const auto color_grn = Ego::Math::Colour4f::parse(0x7F, 0xFF, 0x7F, 0xFF);
    const auto color_blu = Ego::Math::Colour4f::parse(0x7F, 0x7F, 0xFF, 0xFF);
    const auto default_tint = Ego::Math::Colour4f::white();

    //Max search distance in quad tree relative to object position
    const float MAX_SEARCH_DIST = 3.0f * Info<float>::Grid::Size();

    //Max grab distance is 2/3rds of a tile
    const float MAX_DIST_GRAB = Info<float>::Grid::Size() * 0.66f;

    ObjectRef   ichr_b;
    slot_t    slot;
    oct_vec_v2_t mids;
    Vector3f   slot_pos;

    bool retval;

    // valid objects that can be grabbed
    size_t      grab_visible_count = 0;
    std::vector<grab_data_t> grabList;

    // valid objects that cannot be grabbed
    size_t      ungrab_visible_count = 0;
    std::vector<grab_data_t> ungrabList;

    const std::shared_ptr<Object> &pchr_a = _currentModule->getObjectHandler()[ichr_a];
    if (!pchr_a) return false;

    // find the slot from the grip
    slot = grip_offset_to_slot( grip_off );
    if ( slot >= SLOT_COUNT ) return false;

    // Make sure the character doesn't have something already, and that it has hands
    if (_currentModule->getObjectHandler().exists( pchr_a->holdingwhich[slot] ) || !pchr_a->getProfile()->isSlotValid(slot)) {        
        return false;
    }

    //Determine the position of the grip
    mids = pchr_a->slot_cv[slot].getMid();
    slot_pos[kX] = mids[OCT_X];
    slot_pos[kY] = mids[OCT_Y];
    slot_pos[kZ] = mids[OCT_Z];
    slot_pos += pchr_a->getPosition();

    // Go through all characters to find the best match
    std::vector<std::shared_ptr<Object>> nearbyObjects = _currentModule->getObjectHandler().findObjects(slot_pos[kX], slot_pos[kY], MAX_SEARCH_DIST, false);
    for(const std::shared_ptr<Object> &pchr_c : nearbyObjects)
    {
        grab_data_t grabData;
        bool canGrab = true;

        //Skip invalid objects
        if(pchr_c->isTerminated()) {
            continue;
        }

        // do nothing to yourself
        if (pchr_a == pchr_c) continue;

        // Dont do hidden objects
        if (pchr_c->isHidden()) continue;

        // pickpocket not allowed yet
        if ( _currentModule->getObjectHandler().exists( pchr_c->inwhich_inventory ) ) continue;

        // disarm not allowed yet
        if ( ObjectRef::Invalid != pchr_c->attachedto ) continue;

        // do not pick up your mount
        if ( pchr_c->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_c->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        // do not notice completely broken items?
        if (pchr_c->isitem && !pchr_c->isAlive()) continue;

        // reasonable carrying capacity
        if ( pchr_c->phys.weight > pchr_a->phys.weight + FLOAT_TO_FP8(pchr_a->getAttribute(Ego::Attribute::MIGHT)) * INV_FF )
        {
            canGrab = false;
        }

        // grab_people == true allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( !grab_people && !pchr_c->isitem )
        {
            canGrab = false;
        }

        // is the object visible
        grabData.visible = pchr_a->canSeeObject(pchr_c);

        // calculate the distance
        grabData.horizontalDistance = (pchr_c->getPosition() - slot_pos).length();
        grabData.verticalDistance = std::sqrt(Ego::Math::sq( pchr_a->getPosZ() - pchr_c->getPosZ()));
 
        //Figure out if the character is looking towards the object
        grabData.isFacingObject = pchr_a->isFacingLocation(pchr_c->getPosX(), pchr_c->getPosY());

        // Is it too far away to interact with?
        if (grabData.horizontalDistance > MAX_SEARCH_DIST || grabData.verticalDistance > MAX_SEARCH_DIST) continue;

        // visibility affects the max grab distance.
        // if it is not visible then we have to be touching it.
        float maxHorizontalGrabDistance = MAX_DIST_GRAB;
        if ( !grabData.visible )
        {
            maxHorizontalGrabDistance *= 0.5f;
        }

        //Halve grab distance for items behind us
        if(!grabData.isFacingObject && !grab_people) {
            maxHorizontalGrabDistance *= 0.5f;
        }

        //Bigger characters have bigger grab size
        maxHorizontalGrabDistance += pchr_a->bump.size / 4.0f;

        //Double grab distance for monsters that are trying to grapple
        if(grab_people) {
            maxHorizontalGrabDistance *= 2.0f;
        }

        // is it too far away to grab?
        if (grabData.horizontalDistance > maxHorizontalGrabDistance + pchr_a->bump.size / 4.0f && grabData.horizontalDistance > pchr_a->bump.size)
        {
            canGrab = false;
        }

        //Check vertical distance as well
        else
        {
            float maxVerticalGrabDistance = pchr_a->bump.height / 2.0f;

            if(grab_people)
            {
                //This allows very flat creatures like the Carpet Mimics grab people
                maxVerticalGrabDistance = std::max(maxVerticalGrabDistance, MAX_DIST_GRAB);
            }

            if (grabData.verticalDistance > maxVerticalGrabDistance)
            {
                canGrab = false;
            }
        }

        // count the number of objects that are within the max range
        // a difference between the *_total_count and the *_count
        // indicates that some objects were not detectable
        if ( grabData.visible )
        {
            if (canGrab)
            {
                grab_visible_count++;
            }
            else
            {
                ungrab_visible_count++;
            }
        }

        grabData.object = pchr_c;
        if (canGrab)
        {
            grabList.push_back(grabData);
        }
        else
        {
            ungrabList.push_back(grabData);
        }
    }

    // sort the grab list
    if (!grabList.empty())
    {
        std::sort(grabList.begin(), grabList.end(), 
            [](const grab_data_t &a, const grab_data_t &b)
            { 
                float distance = a.horizontalDistance - b.horizontalDistance;
                if(distance <= FLT_EPSILON)
                {
                    distance += a.verticalDistance - b.verticalDistance;
                }

                return distance < 0.0f;
            });
    }

    // try to grab something
    retval = false;
    if (grabList.empty() && ( 0 != grab_visible_count ) )
    {
        // There are items within the "normal" range that could be grabbed
        // but somehow they can't be seen.
        // Generate a billboard that tells the player what the problem is.
        // NOTE: this is not corerect since it could alert a player to an invisible object

        // 5 seconds and blue
        BillboardSystem::get().makeBillboard( ichr_a, "I can't feel anything...", color_blu, default_tint, 3, Billboard::Flags::Fade );

        retval = true;
    }

    if ( !retval )
    {
        for(const grab_data_t &grabData : grabList)
        {
            if (!grabData.visible) {
                continue;
            } 

            bool can_grab = can_grab_item_in_shop(ichr_a, grabData.object->getObjRef());

            if ( can_grab )
            {
                // Stick 'em together and quit
                if ( rv_success == attach_character_to_mount(grabData.object->getObjRef(), ichr_a, grip_off) )
                {
                    if (grab_people)
                    {
                        // Start the slam animation...  ( Be sure to drop!!! )
                        chr_play_action( pchr_a.get(), ACTION_MC + slot, false );
                    }
                    retval = true;
                }
                break;
            }
            else
            {
                // Lift the item a little and quit...
                grabData.object->vel[kZ] = DROPZVEL;
                grabData.object->hitready = true;
                SET_BIT( grabData.object->ai.alert, ALERTIF_DROPPED );
                break;
            }
        }
    }

    if ( !retval )
    {
        Vector3f vforward;

        //---- generate billboards for things that players can interact with
        if (Ego::FeedbackType::None != egoboo_config_t::get().hud_feedback.getValue() && VALID_PLA(pchr_a->is_which_player))
        {
            // things that can be grabbed
            for(const grab_data_t &grabData : grabList)
            {
                ichr_b = grabData.object->getObjRef();
                if (!grabData.visible)
                {
                    // (5 secs and blue)
                    BillboardSystem::get().makeBillboard( ichr_b, "Something...", color_blu, default_tint, 3, Billboard::Flags::Fade );
                }
                else
                {
                    // (5 secs and green)
                    BillboardSystem::get().makeBillboard( ichr_b, grabData.object->getName(true, false, true).c_str(), color_grn, default_tint, 3, Billboard::Flags::Fade );
                }
            }

            // things that can't be grabbed
            for(const grab_data_t &grabData : ungrabList)
            {
                ichr_b = grabData.object->getObjRef();
                if (!grabData.visible)
                {
                    // (5 secs and blue)
                    BillboardSystem::get().makeBillboard( ichr_b, "Something...", color_blu, default_tint, 3, Billboard::Flags::Fade );
                }
                else
                {
                    // (5 secs and red)
                    BillboardSystem::get().makeBillboard( ichr_b, grabData.object->getName(true, false, true).c_str(), color_red, default_tint, 3, Billboard::Flags::Fade );
                }
            }
        }

        //---- if you can't grab anything, activate something using ALERTIF_BUMPED
        if (pchr_a->isPlayer() && !ungrabList.empty())
        {
            // sort the ungrab list
            std::sort(ungrabList.begin(), ungrabList.end(), 
                [](const grab_data_t &a, const grab_data_t &b)
                { 
                    //If objects are basically on top of each other, then sort by vertical distance
                    if(std::abs(a.horizontalDistance - b.horizontalDistance) <= FLT_EPSILON) {
                        return a.verticalDistance < b.verticalDistance;
                    }

                    return a.horizontalDistance < b.horizontalDistance;
                });

            for(const grab_data_t &grabData : ungrabList)
            {
                // only do visible objects
                if (!grabData.visible) continue;

                // only bump the closest character that is in front of the character
                // (ignore vertical displacement)
                if (grabData.isFacingObject && grabData.horizontalDistance < MAX_DIST_GRAB)
                {
                    ai_state_t::set_bumplast(grabData.object->ai, ichr_a);
                    break;
                }
            }
        }
    }

    return retval;
}

float get_chr_mass(Object * pchr)
{
    /// @author BB
    /// @details calculate a "mass" for an object, taking into account possible infinite masses.

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight )
    {
        return -static_cast<float>(CHR_INFINITE_WEIGHT);
    }
    else if ( 0.0f == pchr->phys.bumpdampen )
    {
        return -static_cast<float>(CHR_INFINITE_WEIGHT);
    }
    else
    {
        return pchr->phys.weight / pchr->phys.bumpdampen;
    }
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_update_collision_size( Object * pchr, bool update_matrix )
{
    /// @author BB
    ///
    /// @details use this function to update the pchr->chr_max_cv and  pchr->chr_min_cv with
    ///       values that reflect the best possible collision volume
    ///
    /// @note This function takes quite a bit of time, so it must only be called when the
    /// vertices change because of an animation or because the matrix changes.
    ///
    /// @todo it might be possible to cache the src[] array used in this function.
    /// if the matrix changes, then you would not need to recalculate this data...

    Vector4f  src[16];  // for the upper and lower octagon points
    Vector4f  dst[16];  // for the upper and lower octagon points

    if ( nullptr == pchr ) return rv_error;

    // re-initialize the collision volumes
    pchr->chr_min_cv = oct_bb_t();
    pchr->chr_max_cv = oct_bb_t();
    for ( size_t cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pchr->slot_cv[cnt] = oct_bb_t();
    }

    // make sure the matrix is updated properly
    if ( update_matrix )
    {
        // call chr_update_matrix() but pass in a false value to prevent a recursive call
        if ( rv_error == chr_update_matrix( pchr, false ) )
        {
            return rv_error;
        }
    }

    // make sure the bounding box is calculated properly
    if (gfx_error == chr_instance_t::update_bbox(pchr->inst)) {
        return rv_error;
    }

    // convert the point cloud in the GLvertex array (pchr->inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    oct_bb_t bsrc = pchr->inst.bbox;

    // convert the corners of the level 1 bounding box to a point cloud
    // keep track of the actual number of vertices, in case the object is square
    int vcount = oct_bb_t::to_points(bsrc, src, 16);

    // transform the new point cloud
    Utilities::transform(pchr->inst.matrix, src, dst, vcount);

    // convert the new point cloud into a level 1 bounding box
    oct_bb_t bdst;
    oct_bb_t::points_to_oct_bb(bdst, dst, vcount);

    //---- set the bounding boxes
    pchr->chr_min_cv = bdst;
    pchr->chr_max_cv = bdst;

    oct_bb_t bmin;
    bmin.assign(pchr->bump);

    // only use pchr->bump.size if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideSize() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_X);
        pchr->chr_min_cv.cut(bmin, OCT_Y);

        pchr->chr_max_cv.join(bmin, OCT_X);
        pchr->chr_max_cv.join(bmin, OCT_Y);
    }

    // only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideSizeBig() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_XY);
        pchr->chr_min_cv.cut(bmin, OCT_YX);

        pchr->chr_max_cv.join(bmin, OCT_XY);
        pchr->chr_max_cv.join(bmin, OCT_YX);
    }

    // only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideHeight() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_Z);
        pchr->chr_max_cv.join(bmin, OCT_Z);
    }

    //// raise the upper bound for platforms
    //if ( pchr->platform )
    //{
    //    pchr->chr_max_cv.maxs[OCT_Z] += PLATTOLERANCE;
    //}

    //This makes it easier to jump on top of mounts
    if(pchr->isMount()) {
       pchr->chr_max_cv._maxs[OCT_Z] = std::min<float>(MOUNTTOLERANCE, pchr->chr_max_cv._maxs[OCT_Z]);
       pchr->chr_min_cv._maxs[OCT_Z] = std::min<float>(MOUNTTOLERANCE, pchr->chr_min_cv._maxs[OCT_Z]);
    }

    // calculate collision volumes for various slots
    for ( size_t cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        if ( !pchr->getProfile()->isSlotValid( static_cast<slot_t>(cnt) ) ) continue;

        chr_calc_grip_cv( pchr, GRIP_LEFT, &pchr->slot_cv[cnt], false );

        pchr->chr_max_cv.join(pchr->slot_cv[cnt]);
    }

    // convert the level 1 bounding box to a level 0 bounding box
    oct_bb_t::downgrade(bdst, pchr->bump_stt, pchr->bump, pchr->bump_1);

    return rv_success;
}

egolib_rv attach_character_to_mount( ObjectRef riderRef, ObjectRef mountRef, grip_offset_t grip_off )
{
    /// @author ZZ
    /// @details This function attaches one object (rider) to another object (mount)
    ///          at a certain vertex offset ( grip_off )
    ///   - This function is called as a part of spawning a module, so the rider or the mount may not
    ///     be fully instantiated
    ///   - This function should do very little testing to see if attachment is allowed.
    ///     Most of that checking should be done by the calling function

    // Make sure the character/item is valid
    if (!_currentModule->getObjectHandler().exists(riderRef)) return rv_error;
    Object *rider = _currentModule->getObjectHandler().get(riderRef);

    // Make sure the holder/mount is valid
    if (!_currentModule->getObjectHandler().exists(mountRef)) return rv_error;
    Object *mount = _currentModule->getObjectHandler().get(mountRef);

    //Don't attach a character to itself!
    if (riderRef == mountRef) {
        return rv_fail;
    }

    // do not deal with packed items at this time
    // this would have to be changed to allow for pickpocketing
    if (_currentModule->getObjectHandler().exists(rider->inwhich_inventory) || 
		_currentModule->getObjectHandler().exists(mount->inwhich_inventory)) return rv_fail;

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if (mountRef == rider->dismount_object && rider->dismount_timer > 0) return rv_fail;

    // Figure out which slot this grip_off relates to
    slot_t slot = grip_offset_to_slot(grip_off);

    // Make sure the the slot is valid
    if (!mount->getProfile()->isSlotValid(slot)) return rv_fail;

    // This is a small fix that allows special grabbable mounts not to be mountable while
    // held by another character (such as the magic carpet for example)
    // ( this is an example of a test that should not be done here )
    if (mount->isMount() && _currentModule->getObjectHandler().exists(mount->attachedto)) return rv_fail;

    // Put 'em together
    rider->inwhich_slot       = slot;
    rider->attachedto         = mountRef;
    mount->holdingwhich[slot] = riderRef;

    // set the grip vertices for the irider
    set_weapongrip(riderRef, mountRef, grip_off);

    chr_update_matrix(rider, true);

    rider->setPosition(mat_getTranslate(rider->inst.matrix));

    rider->enviro.inwater  = false;
    rider->jump_timer = JUMPDELAY * 4;

    // Run the held animation
    if (mount->isMount() && (GRIP_ONLY == grip_off))
    {
        // Riding imount
        if (_currentModule->getObjectHandler().exists(rider->holdingwhich[SLOT_LEFT] ) || 
			_currentModule->getObjectHandler().exists(rider->holdingwhich[SLOT_RIGHT] ) )
        {
            // if the character is holding anything, make the animation
            // ACTION_MH == "sitting" so that it dies not look so silly
            chr_play_action(rider, ACTION_MH, true);
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            chr_play_action(rider, ACTION_MI, true);
        }

        // set tehis action to loop
        chr_instance_t::set_action_loop(rider->inst, true);
    }
    else if (rider->isAlive())
    {
        /// @note ZF@> hmm, here is the torch holding bug. Removing
        /// the interpolation seems to fix it...
        chr_play_action(rider, ACTION_MM + slot, false );

        chr_instance_t::remove_interpolation(rider->inst);

        // set the action to keep for items
        if (rider->isItem())
        {
            // Item grab
            chr_instance_t::set_action_keep(rider->inst, true);
        }
    }

    // Set the team
    if (rider->isItem())
    {
        rider->team = mount->team;

        // Set the alert
        if (rider->isAlive())
        {
            SET_BIT(rider->ai.alert, ALERTIF_GRABBED);
        }

        // Lore Master perk identifies everything
        if (mount->hasPerk(Ego::Perks::LORE_MASTER)) {
            rider->getProfile()->makeUsageKnown();
            rider->nameknown = true;
            rider->ammoknown = true;
        }
    }

    if (mount->isMount())
    {
        mount->team = rider->team;

        // Set the alert
        if (!mount->isItem() && mount->isAlive())
        {
            SET_BIT(mount->ai.alert, ALERTIF_GRABBED);
        }
    }

    // It's not gonna hit the floor
    rider->hitready = false;

    return rv_success;
}
