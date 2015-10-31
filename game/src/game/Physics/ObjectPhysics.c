#include "ObjectPhysics.h"
#include "game/game.h"
#include "game/player.h"
#include "game/renderer_2d.h"
#include "egolib/Graphics/ModelDescriptor.hpp"

static void move_one_character_do_floor_friction( Object * pchr );
static void move_one_character_do_voluntary( Object * pchr );
static void move_one_character( Object * pchr );
static void move_one_character_do_z_motion( Object * pchr );
static bool move_one_character_integrate_motion( Object * pchr );

static bool chr_do_latch_button( Object * pchr );
static bool chr_do_latch_attack( Object * pchr, slot_t which_slot );

static void keep_weapons_with_holder(const std::shared_ptr<Object> &pchr);

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

void move_one_character_do_voluntary( Object * pchr )
{
    // do voluntary motion
    if (!pchr || pchr->isTerminated()) return;

    CHR_REF ichr = GET_INDEX_PCHR( pchr );

    if ( !pchr->isAlive() || pchr->getAttribute(Ego::Attribute::ACCELERATION) == 0.00f ) return;

    pchr->enviro.new_v[kX] = pchr->vel[kX];
    pchr->enviro.new_v[kY] = pchr->vel[kY];

    //Mounted?
    if ( pchr->isBeingHeld() ) return;

    float new_ax = 0.0f, new_ay = 0.0f;

    // Character latches for generalized movement
    float dvx = pchr->latch.x;
    float dvy = pchr->latch.y;

    // Reverse movements for daze
    if ( pchr->daze_timer > 0 )
    {
        dvx = -dvx;
        dvy = -dvy;
    }

    // Switch x and y for grog
    if ( pchr->grog_timer > 0 )
    {
        std::swap(dvx, dvy);
    }

    // this is the maximum speed that a character could go under the v2.22 system
    float maxspeed = pchr->getAttribute(Ego::Attribute::ACCELERATION) * Physics::g_environment.airfriction / (1.0f - Physics::g_environment.airfriction);
    float speedBonus = 1.0f;

    //Sprint perk gives +10% movement speed if above 75% life remaining
    if(pchr->hasPerk(Ego::Perks::SPRINT) && pchr->getLife() >= pchr->getAttribute(Ego::Attribute::MAX_LIFE)*0.75f) {
        speedBonus += 0.1f;

        //Uninjured? (Dash perk can give another 10% extra speed)
        if(pchr->hasPerk(Ego::Perks::DASH) && pchr->getAttribute(Ego::Attribute::MAX_LIFE)-pchr->getLife() < 1.0f) {
            speedBonus += 0.1f;
        }
    }

    //Rally Bonus? (+10%)
    if(pchr->hasPerk(Ego::Perks::RALLY) && update_wld < pchr->getRallyDuration()) {
        speedBonus += 0.1f;
    }    

    //Increase movement by 1% per Agility above 10 (below 10 agility reduces movement speed!)
    speedBonus += (pchr->getAttribute(Ego::Attribute::AGILITY)-10.0f) * 0.01f;

    //Now apply speed modifiers
    maxspeed *= speedBonus;

    //Check animation frame freeze movement
    if ( chr_instance_t::get_framefx(pchr->inst) & MADFX_STOP )
    {
        //Allow 50% movement while using Shield and have the Mobile Defence perk
        if(pchr->hasPerk(Ego::Perks::MOBILE_DEFENCE) && ACTION_IS_TYPE(pchr->inst.action_which, P))
        {
            maxspeed *= 0.5f;
        }
        //Allow 50% movement with Mobility perk and attacking with a weapon
        else if(pchr->hasPerk(Ego::Perks::MOBILITY) && pchr->isAttacking())
        {
            maxspeed *= 0.5f;
        }
        else
        {
            //No movement allowed
            maxspeed = 0.0f;
        }
    }

    //Check if AI has limited movement rate
    else if(!pchr->isPlayer())
    {
        maxspeed *= pchr->ai.maxSpeed;
    }

    bool sneak_mode_active = pchr->isStealthed();

    //Reduce speed while stealthed
    if(pchr->isStealthed()) {
        if(pchr->hasPerk(Ego::Perks::SHADE)) {
            maxspeed *= 0.75f;  //Shade allows 75% movement speed while stealthed
        }
        else if(pchr->hasPerk(Ego::Perks::STALKER)) {
            maxspeed *= 0.50f;  //Stalker allows 50% movement speed while stealthed
        }
        else {
            maxspeed *= 0.33f;  //Can only move at 33% speed while stealthed
        }
    }
    
    pchr->enviro.new_v[kX] = pchr->enviro.new_v[kY] = 0.0f;
    if (std::abs(dvx) + std::abs(dvy) > 0.05f)
    {
        pchr->enviro.new_v[kX] = dvx * maxspeed;
        pchr->enviro.new_v[kY] = dvy * maxspeed;
    }

    if ( sneak_mode_active )
    {
        // sneak mode
        pchr->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;
    }
    else
    {
        // non-sneak mode
        pchr->movement_bits = ( unsigned )( ~CHR_MOVEMENT_BITS_SNEAK );
    }

    // do platform friction
    if ( _currentModule->getObjectHandler().exists( pchr->onwhichplatform_ref ) )
    {
        Object * pplat = _currentModule->getObjectHandler().get( pchr->onwhichplatform_ref );

        new_ax += ( pplat->vel[kX] + pchr->enviro.new_v[kX] - ( pchr->vel[kX] ) );
        new_ay += ( pplat->vel[kY] + pchr->enviro.new_v[kY] - ( pchr->vel[kY] ) );
    }
    else
    {
        new_ax += ( pchr->enviro.new_v[kX] - pchr->vel[kX] );
        new_ay += ( pchr->enviro.new_v[kY] - pchr->vel[kY] );
    }

    new_ax *= pchr->enviro.traction;
    new_ay *= pchr->enviro.traction;

    //Slippery environment?
    if(pchr->enviro.is_slippy) {
        new_ax *= Physics::g_environment.icefriction * 0.1f;
        new_ay *= Physics::g_environment.icefriction * 0.1f;
    }

    //Limit movement to the max acceleration
    float accelerationMagnitude = std::sqrt(new_ax*new_ax + new_ay*new_ay);
    if(accelerationMagnitude > pchr->getAttribute(Ego::Attribute::ACCELERATION)) {
        new_ax *= pchr->getAttribute(Ego::Attribute::ACCELERATION) / accelerationMagnitude;
        new_ay *= pchr->getAttribute(Ego::Attribute::ACCELERATION) / accelerationMagnitude;
    }

    //Figure out how to turn around
    if ( 0 != pchr->getAttribute(Ego::Attribute::ACCELERATION) )
    {
        switch ( pchr->turnmode )
        {
            // Get direction from ACTUAL change in velocity
            default:
            case TURNMODE_VELOCITY:
                {
                    if (std::abs(dvx) > TURNSPD || std::abs(dvy) > TURNSPD)
                    {
                        if ( VALID_PLA( pchr->is_which_player ) )
                        {
                            // Players turn quickly
                            pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 2 );
                        }
                        else
                        {
                            // AI turn slowly
                            pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 8 );
                        }
                    }
                }
                break;

            // Get direction from the DESIRED change in velocity
            case TURNMODE_WATCH:
                {
                    if (( std::abs( dvx ) > WATCHMIN || std::abs( dvy ) > WATCHMIN ) )
                    {
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 8 );
                    }
                }
                break;

            // Face the target
            case TURNMODE_WATCHTARGET:
                {
                    if ( ichr != pchr->ai.target )
                    {
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( _currentModule->getObjectHandler().get(pchr->ai.target)->getPosX() - pchr->getPosX() , _currentModule->getObjectHandler().get(pchr->ai.target)->getPosY() - pchr->getPosY() ), 8 );
                    }
                }
                break;

            // Otherwise make it spin
            case TURNMODE_SPIN:
                {
                    pchr->ori.facing_z += SPINRATE;
                }
                break;
        }
    }

    //Update velocity
    pchr->vel[kX] += new_ax;
    pchr->vel[kY] += new_ay;
}

void move_one_character_get_environment( Object * pchr )
{
    if (!pchr || pchr->isTerminated()) return;
    chr_environment_t& enviro = pchr->enviro;

    // determine if the character is standing on a platform
    Object *pplatform = NULL;
    if ( _currentModule->getObjectHandler().exists( pchr->onwhichplatform_ref ) )
    {
        pplatform = _currentModule->getObjectHandler().get( pchr->onwhichplatform_ref );
    }
    ego_mesh_t *mesh = _currentModule->getMeshPointer().get();
    //---- character "floor" level
    float grid_level = mesh->getElevation(PointWorld(pchr->getPosX(), pchr->getPosY()), false );
    float water_level = mesh->getElevation(PointWorld(pchr->getPosX(), pchr->getPosY()), true );

    // chr_set_enviro_grid_level() sets up the reflection level and reflection matrix
    if (grid_level != pchr->enviro.grid_level) {
        pchr->enviro.grid_level = grid_level;

        chr_instance_t::apply_reflection_matrix(pchr->inst, grid_level);
    }

    enviro.grid_lerp  = ( pchr->getPosZ() - grid_level ) / PLATTOLERANCE;
    enviro.grid_lerp  = CLIP( enviro.grid_lerp, 0.0f, 1.0f );

    enviro.water_level = water_level;
    enviro.water_lerp  = ( pchr->getPosZ() - water_level ) / PLATTOLERANCE;
    enviro.water_lerp  = CLIP( enviro.water_lerp, 0.0f, 1.0f );

    // The actual level of the floor underneath the character.
    if ( NULL != pplatform )
    {
        enviro.floor_level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }
    else
    {
        enviro.floor_level = pchr->getAttribute(Ego::Attribute::WALK_ON_WATER) ? water_level : grid_level;
    }

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    enviro.level = enviro.floor_level;
    if ( NULL != pplatform )
    {
        enviro.level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }

    //---- The flying height of the character, the maximum of tile level, platform level and water level
    if ( 0 != mesh->test_fx( pchr->getTile(), MAPFX_WATER ) )
    {
        enviro.fly_level = std::max( enviro.level, water._surface_level );
    }

    if ( enviro.fly_level < 0 )
    {
        enviro.fly_level = 0;  // fly above pits...
    }

    // set the zlerp after we have done everything to the particle's level we care to
    enviro.zlerp = ( pchr->getPosZ() - enviro.level ) / PLATTOLERANCE;
    enviro.zlerp = CLIP( enviro.zlerp, 0.0f, 1.0f );

    enviro.grounded = (!pchr->isFlying() && ( enviro.zlerp < 0.25f ) );

    //---- the "twist" of the floor
    enviro.grid_twist = mesh->get_twist( pchr->getTile() );

    // the "watery-ness" of whatever water might be here
    enviro.is_watery = water._is_water && enviro.inwater;
    enviro.is_slippy = !enviro.is_watery && ( 0 != mesh->test_fx( pchr->getTile(), MAPFX_SLIPPY ) );

    //---- traction
    enviro.traction = 1.0f;
    if ( pchr->isFlying() )
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if ( NULL != pplatform )
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        Vector3f platform_up = Vector3f( 0.0f, 0.0f, 1.0f );

        chr_getMatUp(pplatform, platform_up);
        platform_up.normalize();

        enviro.traction = std::abs(platform_up[kZ]) * ( 1.0f - enviro.zlerp ) + 0.25f * enviro.zlerp;

        if ( enviro.is_slippy )
        {
            enviro.traction /= 4.00f * Physics::g_environment.hillslide * (1.0f - enviro.zlerp) + 1.0f * enviro.zlerp;
        }
    }
    else if ( mesh->grid_is_valid( pchr->getTile() ) )
    {
        enviro.traction = std::abs(g_meshLookupTables.twist_nrm[enviro.grid_twist][kZ] ) * ( 1.0f - enviro.zlerp ) + 0.25f * enviro.zlerp;

        if ( enviro.is_slippy )
        {
            enviro.traction /= 4.00f * Physics::g_environment.hillslide * (1.0f - enviro.zlerp) + 1.0f * enviro.zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if (enviro.is_watery)
    {
        //Athletics perk halves penality for moving in water
        if(pchr->hasPerk(Ego::Perks::ATHLETICS)) {
            enviro.fluid_friction_vrt  = (Physics::g_environment.waterfriction + Physics::g_environment.airfriction)*0.5f;
            enviro.fluid_friction_hrz  = (Physics::g_environment.waterfriction + Physics::g_environment.airfriction)*0.5f;
        }
        else {
            enviro.fluid_friction_vrt = Physics::g_environment.waterfriction;
            enviro.fluid_friction_hrz = Physics::g_environment.waterfriction;            
        }

    }
    else if ( NULL != pplatform )
    {
        enviro.fluid_friction_hrz = 1.0f;
        enviro.fluid_friction_vrt = 1.0f;
    }
    else
    {
        // like real-life air friction
        enviro.fluid_friction_hrz = Physics::g_environment.airfriction;
        enviro.fluid_friction_vrt = Physics::g_environment.airfriction;            
    }

    //---- friction
    enviro.friction_hrz = 1.0f;
    if ( pchr->isFlying() )
    {
        if ( pchr->platform )
        {
            // override the z friction for platforms.
            // friction in the z direction will make the bouncing stop
            enviro.fluid_friction_vrt = 1.0f;
        }
    }
    else
    {
        // Make the characters slide
        float temp_friction_xy = Physics::g_environment.noslipfriction;
        if ( mesh->grid_is_valid( pchr->getTile() ) && enviro.is_slippy )
        {
            // It's slippy all right...
            temp_friction_xy = Physics::g_environment.slippyfriction;
        }

        enviro.friction_hrz = enviro.zlerp * 1.0f + ( 1.0f - enviro.zlerp ) * temp_friction_xy;
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
        if (( _currentModule->getObjectHandler().exists( pchr->attachedto ) || pchr->jumpready || pchr->jumpnumber > 0 ) && pchr->jump_timer > 0 ) pchr->jump_timer--;

        // Do ground hits
        if ( enviro.grounded && pchr->vel[kZ] < -STOPBOUNCING && pchr->hitready )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_HITGROUND );
            pchr->hitready = false;
        }

        // Special considerations for slippy surfaces
        if ( enviro.is_slippy )
        {
            if (g_meshLookupTables.twist_flat[enviro.grid_twist] )
            {
                // Reset jumping on flat areas of slippiness
                if ( enviro.grounded && 0 == pchr->jump_timer )
                {
                    pchr->jumpnumber = pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);
                }
            }
        }
        else if ( enviro.grounded && 0 == pchr->jump_timer )
        {
            // Reset jumping
            pchr->jumpnumber = pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);
        }
    }

    // add in something for the "ground speed"
    if ( NULL == pplatform )
    {
        enviro.floor_speed = Vector3f::zero();
    }
    else
    {
        enviro.floor_speed = pplatform->vel;
    }

}

//--------------------------------------------------------------------------------------------
void move_one_character_do_floor_friction( Object * pchr )
{
    /// @author BB
    /// @details Friction is complicated when you want to have sliding characters :P

    if (!pchr || pchr->isTerminated()) return;
    chr_environment_t * penviro = &( pchr->enviro );

    if ( pchr->isFlying() ) return;

    // assume the best
    Vector3f floor_acc = Vector3f::zero();
    float temp_friction_xy = 1.0f;
    Vector3f vup = Vector3f(0.0f, 0.0f, 1.0f);

    // figure out the acceleration due to the current "floor"
    const std::shared_ptr<Object> &platform = _currentModule->getObjectHandler()[pchr->onwhichplatform_ref];
    if (platform != nullptr)
    {
        chr_getMatUp(platform.get(), vup);
        temp_friction_xy = std::min(1.0f, penviro->friction_hrz - PLATFORM_STICKINESS);
        floor_acc = platform->vel - pchr->vel;
    }
    else if (!pchr->isAlive() || pchr->isItem())
    {
        temp_friction_xy = 0.5f;
        floor_acc = -pchr->vel;

        if ( TWIST_FLAT != penviro->grid_twist )
        {
            vup = g_meshLookupTables.twist_nrm[penviro->grid_twist];
        }

    }
    else
    {
        temp_friction_xy = penviro->friction_hrz;
        floor_acc = -pchr->vel;

        if ( TWIST_FLAT != penviro->grid_twist )
        {
            vup = g_meshLookupTables.twist_nrm[penviro->grid_twist];
        }
    }

    floor_acc *= 1.0f - penviro->zlerp;

    // reduce the volountary acceleration peopendicular to the direction of motion?
    if (floor_acc.length_abs() > 0.0f)
    {
        Vector3f acc_para, acc_perp;
        Vector3f vfront;

        // get the direction of motion
        vfront = mat_getChrForward(pchr->inst.matrix);
        vfront.normalize();

        // decompose the acceleration into parallel and perpendicular components
        fvec3_decompose(floor_acc, vfront, acc_para, acc_perp);

        // re-compose the acceleration with 1/2 of the perpendicular taken away
        floor_acc = acc_perp * 0.5f;
        floor_acc += acc_para;
    }

    // the first guess about the floor friction
    Vector3f fric_floor = floor_acc * (penviro->traction * (1.0f - temp_friction_xy));

    // the total "friction" with to the floor
    Vector3f fric = fric_floor + penviro->acc;

    // limit the friction to whatever is horizontal to the mesh
    if (std::abs(vup[kZ]) >= 1.0f)
    {
        fric[kZ] = 0.0f;
        floor_acc[kZ] = 0.0f;
    }
    else
    {
        Vector3f acc_perp, acc_para;

        fvec3_decompose(fric, vup, acc_perp, acc_para);
        fric = acc_para;

        fvec3_decompose(floor_acc, vup, acc_perp, acc_para);
        floor_acc = acc_para;
    }        

    // test to see if the player has any more friction left?
    penviro->is_slipping = (fric.length_abs() > penviro->friction_hrz);

    if (penviro->is_slipping)
    {
        penviro->traction *= 0.5f;
        temp_friction_xy = std::sqrt( temp_friction_xy );

        // the first guess about the floor friction
        fric_floor = floor_acc * (penviro->traction * (1.0f - temp_friction_xy));
    }

    // Apply the floor friction
    pchr->vel += fric_floor;

    // Apply fluid friction from last time
    if(!penviro->is_slippy) {
        pchr->vel[kX] -= pchr->vel[kX] * (1.0f - penviro->fluid_friction_hrz);
        pchr->vel[kY] -= pchr->vel[kY] * (1.0f - penviro->fluid_friction_hrz);
        pchr->vel[kZ] -= pchr->vel[kZ] * (1.0f - penviro->fluid_friction_vrt);
    }
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_z_motion( Object * pchr )
{
    if (!pchr || pchr->isTerminated()) return;

    //---- do z acceleration
    if ( pchr->isFlying() )
    {
        pchr->vel[kZ] += ( pchr->enviro.fly_level + pchr->getAttribute(Ego::Attribute::FLY_TO_HEIGHT) - pchr->getPosZ() ) * FLYDAMPEN;
    }

    else if (
        pchr->enviro.is_slippy && ( pchr->enviro.grid_twist != TWIST_FLAT ) &&
        ( CHR_INFINITE_WEIGHT != pchr->phys.weight ) && ( pchr->enviro.grid_lerp <= pchr->enviro.zlerp ) )
    {
        // Slippy hills make characters slide

        Vector3f   gperp;    // gravity perpendicular to the mesh
        Vector3f   gpara;    // gravity parallel      to the mesh (what pushes you)

        // RELATIVE TO THE GRID, since you might be riding a platform!
        float     loc_zlerp = pchr->enviro.grid_lerp;

        gpara = g_meshLookupTables.twist_vel[pchr->enviro.grid_twist];

        gperp[kX] = 0       - gpara[kX];
        gperp[kY] = 0       - gpara[kY];
        gperp[kZ] = Physics::g_environment.gravity - gpara[kZ];

        pchr->vel += gpara * ( 1.0f - loc_zlerp ) + gperp * loc_zlerp;
    }
    else
    {
        pchr->vel[kZ] += pchr->enviro.zlerp * Physics::g_environment.gravity;
    }
}

//--------------------------------------------------------------------------------------------
bool move_one_character_integrate_motion( Object * pchr )
{
    /// @author BB
    /// @details Figure out the next position of the character.
    ///    Include collisions with the mesh in this step.

    Vector3f tmp_pos;

    if (!pchr || pchr->isTerminated()) return false;

    //Are we being held?
    if (pchr->isBeingHeld())
    {
        //Always set our position to that of our holder
        pchr->setPosition(_currentModule->getObjectHandler()[pchr->attachedto]->getPosition());
        return true;
    }

    tmp_pos = pchr->getPosition();;

    // interaction with the mesh
    //if ( std::abs( pchr->vel[kZ] ) > 0.0f )
    {
        const float vert_offset = RAISE * 0.25f;
        float grid_level = pchr->enviro.grid_level + vert_offset + 5;

        tmp_pos[kZ] += pchr->vel[kZ];
        LOG_NAN( tmp_pos[kZ] );
        if ( tmp_pos[kZ] < grid_level )
        {
            if ( std::abs( pchr->vel[kZ] ) < STOPBOUNCING )
            {
                pchr->vel[kZ] = 0.0f;
                tmp_pos[kZ] = grid_level;
            }
            else
            {
                if ( pchr->vel[kZ] < 0.0f )
                {
                    float diff = grid_level - tmp_pos[kZ];

                    pchr->vel[kZ] *= -pchr->phys.bumpdampen;
                    diff        *= -pchr->phys.bumpdampen;

                    tmp_pos[kZ] = std::max( tmp_pos[kZ] + diff, grid_level );
                }
                else
                {
                    tmp_pos[kZ] = grid_level;
                }
            }
        }
    }

    // fixes to the z-position
    if (pchr->isFlying())
    {
        if ( tmp_pos[kZ] < 0.0f ) tmp_pos[kZ] = 0.0f;  // Don't fall in pits...
    }


    //if (std::abs(pchr->vel[kX]) + std::abs(pchr->vel[kY]) > 0.0f)
    {
        mesh_wall_data_t wdata;

        float old_x = tmp_pos[kX]; LOG_NAN( old_x );
        float old_y = tmp_pos[kY]; LOG_NAN( old_y );

        float new_x = old_x + pchr->vel[kX]; LOG_NAN( new_x );
        float new_y = old_y + pchr->vel[kY]; LOG_NAN( new_y );

        tmp_pos[kX] = new_x;
        tmp_pos[kY] = new_y;

        //Wall collision?
        if ( EMPTY_BIT_FIELD != pchr->test_wall( tmp_pos, &wdata ) )
        {            
            Vector2f nrm;
            float   pressure;

            pchr->hit_wall(tmp_pos, nrm, &pressure, &wdata);

            // how is the character hitting the wall?
            if (pressure > 0.0f)
            {
                //Figure out last safe position
                Vector2f safePos; 
                safePos[kX] = pchr->getSafePosition()[kX];
                safePos[kY] = pchr->getSafePosition()[kY];

                //Calculate velocity vector perpendicular from wall normal
                Vector2f v_perp = Vector2f::zero();
                float nrm2 = nrm.dot(nrm);
                if (0.0f != nrm2) {
                    float dot = Vector2f(pchr->vel[kX], pchr->vel[kY]).dot(nrm);
                    v_perp = nrm * (dot / nrm2);
                }

                //Bounce velocity of normal
                pchr->vel[kX] = pchr->vel[kX] - v_perp[kX] * (1.0f-pchr->phys.bumpdampen);
                pchr->vel[kY] = pchr->vel[kY] - v_perp[kY] * (1.0f-pchr->phys.bumpdampen);

                //Add additional pressure perpendicular from wall depending on how far inside wall we are
                float safeDistance = (Vector2f(tmp_pos[kX], tmp_pos[kY]) - safePos).length() * std::max(0.1f, 1.0f-pchr->phys.bumpdampen);
                pchr->vel[kX] += safeDistance * nrm[kX] * pressure;
                pchr->vel[kY] += safeDistance * nrm[kY] * pressure;

                //Apply correction
                tmp_pos[kX] += pchr->vel[kX];
                tmp_pos[kY] += pchr->vel[kY];
            }
        }
    }

    pchr->setPosition(tmp_pos);

    return true;
}

//--------------------------------------------------------------------------------------------
bool chr_do_latch_button( Object * pchr )
{
    /// @author BB
    /// @details Character latches for generalized buttons

    bool attack_handled;

    if (!pchr || pchr->isTerminated()) return false;
    CHR_REF ichr = pchr->getCharacterID();

    if ( !pchr->isAlive() || pchr->latch.b.none() ) return true;

    const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();

    if ( pchr->latch.b[LATCHBUTTON_JUMP] && 0 == pchr->jump_timer )
    {

        //Jump from our mount
        if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
        {
            pchr->detatchFromHolder(true, true);
            detach_character_from_platform( pchr );

            pchr->jump_timer = JUMPDELAY;
            if ( pchr->isFlying() )
            {
                pchr->vel[kZ] += DISMOUNTZVELFLY;
            }
            else
            {
                pchr->vel[kZ] += DISMOUNTZVEL;
            }

            pchr->setPosition(pchr->getPosX(), pchr->getPosY(), pchr->getPosZ() + pchr->vel[kZ]);

            if ( pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) != JUMPINFINITE && 0 != pchr->jumpnumber )
                pchr->jumpnumber--;

            // Play the jump sound
            AudioSystem::get().playSound(pchr->getPosition(), profile->getJumpSound());
        }

        //Normal jump
        else if ( 0 != pchr->jumpnumber && !pchr->isFlying() )
        {
            if ( 1 != pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) || pchr->jumpready )
            {
                //Exit stealth unless character has Stalker Perk
                if(!pchr->hasPerk(Ego::Perks::STALKER)) {
                    pchr->deactivateStealth();
                }

                // Make the character jump
                pchr->hitready = true;
                if ( pchr->enviro.inwater || pchr->enviro.is_slippy )
                {
                    pchr->jump_timer = JUMPDELAY * 4;         //To prevent 'bunny jumping' in water
                    pchr->vel[kZ] += WATERJUMP;
                }
                else
                {
                    pchr->jump_timer = JUMPDELAY;
                    pchr->vel[kZ] += pchr->getAttribute(Ego::Attribute::JUMP_POWER) * 1.5f;
                }

                pchr->jumpready = false;
                if ( pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) != JUMPINFINITE ) pchr->jumpnumber--;

                // Set to jump animation if not doing anything better
                if ( pchr->inst.action_ready )
                {
                    chr_play_action( pchr, ACTION_JA, true );
                }

                // Play the jump sound (Boing!)
                AudioSystem::get().playSound(pchr->getPosition(), profile->getJumpSound());
            }
        }

    }
    if ( pchr->latch.b[LATCHBUTTON_PACKLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        Inventory::swap_item( ichr, pchr->getInventory().getFirstFreeSlotNumber(), SLOT_LEFT, false );
    }
    if ( pchr->latch.b[LATCHBUTTON_PACKRIGHT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        Inventory::swap_item( ichr, pchr->getInventory().getFirstFreeSlotNumber(), SLOT_RIGHT, false );
    }

    if ( pchr->latch.b[LATCHBUTTON_ALTLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = GRABDELAY;
        if ( !pchr->getLeftHandItem() )
        {
            // Grab left
            if(!pchr->getProfile()->getModel()->isActionValid(ACTION_ME)) {
                //No grab animation valid
                character_grab_stuff( ichr, GRIP_LEFT, false );
            }
            else {
                chr_play_action( pchr, ACTION_ME, false );
            }
        }
        else
        {
            // Drop left
            chr_play_action( pchr, ACTION_MA, false );
        }
    }
    if ( pchr->latch.b[LATCHBUTTON_ALTRIGHT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_ALTRIGHT;

        pchr->reload_timer = GRABDELAY;
        if ( !pchr->getRightHandItem() )
        {
            // Grab right
            if(!pchr->getProfile()->getModel()->isActionValid(ACTION_MF)) {
                //No grab animation valid
                character_grab_stuff( ichr, GRIP_RIGHT, false );
            }
            else {
                chr_play_action( pchr, ACTION_MF, false );
            }
        }
        else
        {
            // Drop right
            chr_play_action( pchr, ACTION_MB, false );
        }
    }

    // LATCHBUTTON_LEFT and LATCHBUTTON_RIGHT are mutually exclusive
    attack_handled = false;
    if ( !attack_handled && pchr->latch.b[LATCHBUTTON_LEFT] && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_LEFT;
        attack_handled = chr_do_latch_attack( pchr, SLOT_LEFT );
    }
    if ( !attack_handled && pchr->latch.b[LATCHBUTTON_RIGHT] && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_RIGHT;

        attack_handled = chr_do_latch_attack( pchr, SLOT_RIGHT );
    }

    return true;
}

bool chr_do_latch_attack( Object * pchr, slot_t which_slot )
{
    CHR_REF ichr, iweapon;

    int    base_action, hand_action, action;
    bool action_valid, allowedtoattack;

    bool retval = false;

    if (!pchr || pchr->isTerminated()) return false;
    ichr = GET_INDEX_PCHR( pchr );


    if (which_slot >= SLOT_COUNT) return false;

    // Which iweapon?
    iweapon = pchr->holdingwhich[which_slot];
    if ( !_currentModule->getObjectHandler().exists( iweapon ) )
    {
        // Unarmed means character itself is the iweapon
        iweapon = ichr;
    }
    Object *pweapon     = _currentModule->getObjectHandler().get(iweapon);
    const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

    //No need to continue if we have an attack cooldown
    if ( 0 != pweapon->reload_timer ) return false;

    // grab the iweapon's action
    base_action = weaponProfile->getWeaponAction();
    hand_action = pchr->getProfile()->getModel()->randomizeAction( base_action, which_slot );

    // see if the character can play this action
    action       = pchr->getProfile()->getModel()->getAction(hand_action);
    action_valid = TO_C_BOOL( ACTION_COUNT != action );

    // Can it do it?
    allowedtoattack = true;

    // First check if reload time and action is okay
    if ( !action_valid )
    {
        allowedtoattack = false;
    }
    else
    {
        // Then check if a skill is needed
        if ( weaponProfile->requiresSkillIDToUse() )
        {
            if (!pchr->hasSkillIDSZ(pweapon->getProfile()->getIDSZ(IDSZ_SKILL)))
            {
                allowedtoattack = false;
            }
        }
    }

    // Don't allow users with kursed weapon in the other hand to use longbows
    if ( allowedtoattack && ACTION_IS_TYPE( action, L ) )
    {
        const std::shared_ptr<Object> &offhandItem = which_slot == SLOT_LEFT ? pchr->getLeftHandItem() : pchr->getRightHandItem();
        if(offhandItem && offhandItem->iskursed) allowedtoattack = false;
    }

    if ( !allowedtoattack )
    {
        // This character can't use this iweapon
        pweapon->reload_timer = ONESECOND;
        if (pchr->getShowStatus() || egoboo_config_t::get().debug_developerMode_enable.getValue())
        {
            // Tell the player that they can't use this iweapon
            DisplayMsg_printf( "%s can't use this item...", pchr->getName(false, true, true).c_str());
        }
        return false;
    }

    if ( ACTION_DA == action )
    {
        allowedtoattack = false;
        if ( 0 == pweapon->reload_timer )
        {
            SET_BIT( pweapon->ai.alert, ALERTIF_USED );
        }
    }

    // deal with your mount (which could steal your attack)
    if ( allowedtoattack )
    {
        // Rearing mount
        const std::shared_ptr<Object> &pmount = _currentModule->getObjectHandler()[pchr->attachedto];

        if (pmount)
        {
            const std::shared_ptr<ObjectProfile> &mountProfile = pmount->getProfile();

            // let the mount steal the rider's attack
            if (!mountProfile->riderCanAttack()) allowedtoattack = false;

            // can the mount do anything?
            if ( pmount->isMount() && pmount->isAlive() )
            {
                // can the mount be told what to do?
                if ( !VALID_PLA( pmount->is_which_player ) && pmount->inst.action_ready )
                {
                    if ( !ACTION_IS_TYPE( action, P ) || !mountProfile->riderCanAttack() )
                    {
                        chr_play_action( pmount.get(), Random::next((int)ACTION_UA, ACTION_UA + 1), false );
                        SET_BIT( pmount->ai.alert, ALERTIF_USED );
                        pchr->ai.lastitemused = pmount->getCharacterID();

                        retval = true;
                    }
                }
            }
        }
    }

    // Attack button
    if ( allowedtoattack )
    {
        //Attacking or using an item disables stealth
        pchr->deactivateStealth();

        if ( pchr->inst.action_ready && action_valid )
        {
            //Check if we are attacking unarmed and cost mana to do so
            if(iweapon == pchr->getCharacterID())
            {
                if(pchr->getProfile()->getUseManaCost() <= pchr->getMana())
                {
                    pchr->costMana(pchr->getProfile()->getUseManaCost(), pchr->getCharacterID());
                }
                else
                {
                    allowedtoattack = false;
                }
            }

            if(allowedtoattack)
            {
                // randomize the action
                action = pchr->getProfile()->getModel()->randomizeAction( action, which_slot );

                // make sure it is valid
                action = pchr->getProfile()->getModel()->getAction(action);

                if ( ACTION_IS_TYPE( action, P ) )
                {
                    // we must set parry actions to be interrupted by anything
                    chr_play_action( pchr, action, true );
                }
                else
                {
                    float agility = pchr->getAttribute(Ego::Attribute::AGILITY);

                    chr_play_action( pchr, action, false );

                    // Make the weapon animate the attack as well as the character holding it
                    if ( iweapon != ichr )
                    {
                        chr_play_action( pweapon, ACTION_MJ, false );
                    }

                    //Crossbow Mastery increases XBow attack speed by 30%
                    if(pchr->hasPerk(Ego::Perks::CROSSBOW_MASTERY) && 
                       pweapon->getProfile()->getIDSZ(IDSZ_PARENT) == MAKE_IDSZ('X','B','O','W')) {
                        agility *= 1.30f;
                    }

                    //Determine the attack speed (how fast we play the animation)
                    pchr->inst.rate  = 0.80f;                                 //base attack speed
                    pchr->inst.rate += std::min(3.00f, agility * 0.02f);      //every Agility increases base attack speed by 2%

                    //If Quick Strike perk triggers then we have fastest possible attack (10% chance)
                    if(pchr->hasPerk(Ego::Perks::QUICK_STRIKE) && pweapon->getProfile()->isMeleeWeapon() && Random::getPercent() <= 10) {
                        pchr->inst.rate = 3.00f;
                        chr_make_text_billboard(pchr->getCharacterID(), "Quick Strike!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
                    }

                    //Add some reload time as a true limit to attacks per second
                    //Dexterity decreases the reload time for all weapons. We could allow other stats like intelligence
                    //reduce reload time for spells or gonnes here.
                    else if ( !weaponProfile->hasFastAttack() )
                    {
                        int base_reload_time = -agility;
                        if ( ACTION_IS_TYPE( action, U ) )      base_reload_time += 50;     //Unarmed  (Fists)
                        else if ( ACTION_IS_TYPE( action, T ) ) base_reload_time += 55;     //Thrust   (Spear)
                        else if ( ACTION_IS_TYPE( action, C ) ) base_reload_time += 85;     //Chop     (Axe)
                        else if ( ACTION_IS_TYPE( action, S ) ) base_reload_time += 65;     //Slice    (Sword)
                        else if ( ACTION_IS_TYPE( action, B ) ) base_reload_time += 70;     //Bash     (Mace)
                        else if ( ACTION_IS_TYPE( action, L ) ) base_reload_time += 60;     //Longbow  (Longbow)
                        else if ( ACTION_IS_TYPE( action, X ) ) base_reload_time += 130;    //Xbow     (Crossbow)
                        else if ( ACTION_IS_TYPE( action, F ) ) base_reload_time += 60;     //Flinged  (Unused)

                        //it is possible to have so high dex to eliminate all reload time
                        if ( base_reload_time > 0 ) pweapon->reload_timer += base_reload_time;
                    }
                }

                // let everyone know what we did
                pchr->ai.lastitemused = iweapon;

                /// @note ZF@> why should there any reason the weapon should NOT be alerted when it is used?
                // grab the MADFX_* flags for this action
//                BIT_FIELD action_madfx = getProfile()->getModel()->getActionFX(action);
//                if ( iweapon == ichr || HAS_NO_BITS( action, MADFX_ACTLEFT | MADFX_ACTRIGHT ) )
                {
                    SET_BIT( pweapon->ai.alert, ALERTIF_USED );
                }

                retval = true;
            }
        }
    }

    //Reset boredom timer if the attack succeeded
    if ( retval )
    {
        pchr->bore_timer = BORETIME;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool character_grab_stuff( const CHR_REF ichr_a, grip_offset_t grip_off, bool grab_people )
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

    CHR_REF   ichr_b;
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
    if ( _currentModule->getObjectHandler().exists( pchr_a->holdingwhich[slot] ) || !pchr_a->getProfile()->isSlotValid(slot) )
        return false;

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
        if ( pchr_c->isHidden() ) continue;

        // pickpocket not allowed yet
        if ( _currentModule->getObjectHandler().exists( pchr_c->inwhich_inventory ) ) continue;

        // disarm not allowed yet
        if ( INVALID_CHR_REF != pchr_c->attachedto ) continue;

        // do not pick up your mount
        if ( pchr_c->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_c->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        // do not notice completely broken items?
        if ( pchr_c->isitem && !pchr_c->isAlive() ) continue;

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

        //Halve grab distance for objects behind us
        if(!grabData.isFacingObject) {
            maxHorizontalGrabDistance *= 0.5f;
        }

        //Bigger characters have bigger grab size
        maxHorizontalGrabDistance += pchr_a->bump.size / 4.0f;

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
        chr_make_text_billboard( ichr_a, "I can't feel anything...", color_blu, default_tint, 3, Billboard::Flags::Fade );

        retval = true;
    }

    if ( !retval )
    {
        for(const grab_data_t &grabData : grabList)
        {
            if (!grabData.visible) {
                continue;
            } 

            bool can_grab = can_grab_item_in_shop(ichr_a, grabData.object->getCharacterID());

            if ( can_grab )
            {
                // Stick 'em together and quit
                if ( rv_success == attach_character_to_mount(grabData.object->getCharacterID(), ichr_a, grip_off) )
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
                ichr_b = grabData.object->getCharacterID();
                if (!grabData.visible)
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 3, Billboard::Flags::Fade );
                }
                else
                {
                    // (5 secs and green)
                    chr_make_text_billboard( ichr_b, grabData.object->getName(true, false, true).c_str(), color_grn, default_tint, 3, Billboard::Flags::Fade );
                }
            }

            // things that can't be grabbed
            for(const grab_data_t &grabData : ungrabList)
            {
                ichr_b = grabData.object->getCharacterID();
                if (!grabData.visible)
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 3, Billboard::Flags::Fade );
                }
                else
                {
                    // (5 secs and red)
                    chr_make_text_billboard( ichr_b, grabData.object->getName(true, false, true).c_str(), color_red, default_tint, 3, Billboard::Flags::Fade );
                }
            }
        }

        //---- if you can't grab anything, activate something using ALERTIF_BUMPED
        if ( VALID_PLA( pchr_a->is_which_player ) && !ungrabList.empty() )
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

//--------------------------------------------------------------------------------------------
void move_one_character( Object * pchr )
{
    if (!pchr || pchr->isTerminated()) return;

    if ( _currentModule->getObjectHandler().exists( pchr->inwhich_inventory ) ) return;

    // save the velocity and acceleration from the last time-step
    pchr->enviro.vel = pchr->getPosition() - pchr->pos_old;
    pchr->enviro.acc = pchr->vel - pchr->vel_old;

    // Character's old location
    pchr->pos_old = pchr->getPosition();
    pchr->vel_old          = pchr->vel;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    pchr->enviro.new_v[kX] = pchr->vel[kX];
    pchr->enviro.new_v[kY] = pchr->vel[kY];

    move_one_character_get_environment( pchr );

    // do friction with the floor before voluntary motion
    move_one_character_do_floor_friction( pchr );

    move_one_character_do_voluntary( pchr );

    chr_do_latch_button( pchr );

    move_one_character_do_z_motion( pchr );

    move_one_character_integrate_motion( pchr );

    move_one_character_do_animation( pchr );

    // Characters with sticky butts lie on the surface of the mesh
    if ( pchr->getProfile()->hasStickyButt() || !pchr->isAlive() )
    {
        float fkeep = ( 7 + pchr->enviro.zlerp ) / 8.0f;
        float fnew  = ( 1 - pchr->enviro.zlerp ) / 8.0f;

        if ( fnew > 0 )
        {
            pchr->ori.map_twist_facing_x = pchr->ori.map_twist_facing_x * fkeep + g_meshLookupTables.twist_facing_x[pchr->enviro.grid_twist] * fnew;
            pchr->ori.map_twist_facing_y = pchr->ori.map_twist_facing_y * fkeep + g_meshLookupTables.twist_facing_y[pchr->enviro.grid_twist] * fnew;
        }
    }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holder(const std::shared_ptr<Object> &pchr)
{
    /// @author ZZ
    /// @details This function keeps weapons near their holders

    CHR_REF iattached = pchr->attachedto;
    if ( _currentModule->getObjectHandler().exists( iattached ) )
    {
        Object * pattached = _currentModule->getObjectHandler().get( iattached );

        // Keep in hand weapons with iattached
        if ( chr_matrix_valid( pchr.get() ) )
        {
            pchr->setPosition(mat_getTranslate(pchr->inst.matrix));
        }
        else
        {
            pchr->setPosition(pattached->getPosition());
        }

        pchr->ori.facing_z = pattached->ori.facing_z;

        // Copy this stuff ONLY if it's a weapon, not for mounts
        if ( pattached->getProfile()->transferBlending() && pchr->isitem )
        {

            // Items become partially invisible in hands of players
            if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.alpha )
            {
                pchr->setAlpha(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if ( 255 == pchr->getProfile()->getAlpha() )
                {
                    pchr->setAlpha(pattached->inst.alpha);
                }
                else
                {
                    pchr->setAlpha(pchr->getProfile()->getAlpha());
                }
            }

            // Do light too
            if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.light )
            {
                pchr->setLight(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if ( 255 == pchr->getProfile()->getLight())
                {
                    pchr->setLight(pattached->inst.light);
                }
                else
                {
                    pchr->setLight(pchr->getProfile()->getLight());
                }
            }
        }
    }
    else
    {
        pchr->attachedto = INVALID_CHR_REF;

        // Keep inventory with iattached
        if ( !_currentModule->getObjectHandler().exists( pchr->inwhich_inventory ) )
        {
            for(const std::shared_ptr<Object> pitem : pchr->getInventory().iterate())
            {
                pitem->setPosition(pchr->getPosition());

                // Copy olds to make SendMessageNear work
                pitem->pos_old = pchr->pos_old;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_all_characters()
{
    /// @author ZZ
    /// @details This function handles character physics

    chr_stoppedby_tests = 0;

    // Move every character
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        // prime the environment
        object->enviro.ice_friction = Physics::g_environment.icefriction;

        move_one_character( object.get() );

        chr_update_matrix( object.get(), true );
        keep_weapons_with_holder(object);
    }
}

bool detach_character_from_platform( Object * pchr )
{
    /// @author BB
    /// @details attach a character to a platform
    ///
    /// @note the function move_one_character_get_environment() has already been called from within the
    ///  move_one_character() function, so the environment has already been determined this round

    // verify that we do not have two dud pointers
    if (!pchr || pchr->isTerminated()) {
        return false;
    }

    // save some values
    const std::shared_ptr<Object> &oldPlatform = _currentModule->getObjectHandler()[pchr->onwhichplatform_ref];

    // undo the attachment
    pchr->onwhichplatform_ref    = INVALID_CHR_REF;
    pchr->onwhichplatform_update = 0;
    pchr->targetplatform_ref     = INVALID_CHR_REF;
    pchr->targetplatform_level   = -1e32;

    // adjust the platform weight, if necessary
    if (oldPlatform) {
        oldPlatform->holdingweight -= pchr->phys.weight;
    }

    // update the character-platform properties
    move_one_character_get_environment( pchr );

    // update the character jumping
    pchr->jumpready = pchr->enviro.grounded;
    if ( pchr->jumpready )
    {
        pchr->jumpnumber = pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);
    }

    return true;
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
    oct_bb_t::ctor(pchr->chr_min_cv);
    oct_bb_t::ctor(pchr->chr_max_cv);
    for ( size_t cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        oct_bb_t::ctor(pchr->slot_cv[cnt]);
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
    int vcount = oct_bb_to_points(bsrc, src, 16);

    // transform the new point cloud
    Utilities::transform(pchr->inst.matrix, src, dst, vcount);

    // convert the new point cloud into a level 1 bounding box
    oct_bb_t bdst;
    points_to_oct_bb(bdst, dst, vcount);

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