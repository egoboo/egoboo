#include "object_physics.h"
#include "game/game.h"
#include "game/player.h"
#include "game/renderer_2d.h"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/Physics/PhysicalConstants.hpp"
#include "game/Core/GameEngine.hpp"

#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
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

    // chr_set_enviro_grid_level() sets up the reflection level and reflection matrix
    if (grid_level != pchr->enviro.grid_level) {
        pchr->enviro.grid_level = grid_level;

        chr_instance_t::apply_reflection_matrix(pchr->inst, grid_level);
    }

    // The actual level of the floor underneath the character.
    if (pplatform)
    {
        enviro.floor_level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }
    else
    {
        enviro.floor_level = pchr->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 ? mesh->getElevation(Vector2f(pchr->getPosX(), pchr->getPosY()), true) : grid_level;
    }

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    if (pplatform)
    {
        enviro.level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }
    else {
        enviro.level = enviro.floor_level;
    }

    //---- The flying height of the character, the maximum of tile level, platform level and water level
    if (pchr->isOnWaterTile())
    {
        enviro.fly_level = std::max(enviro.level, water._surface_level);
    }

    // fly above pits...
    if (enviro.fly_level < 0)
    {
        enviro.fly_level = 0;
    }

    // set the zlerp
    enviro.zlerp = (pchr->getPosZ() - enviro.level) / PLATTOLERANCE;
    enviro.zlerp = Ego::Math::constrain(enviro.zlerp, 0.0f, 1.0f);

    enviro.grounded = !pchr->isFlying() && enviro.zlerp <= 0.25f;

    // the "watery-ness" of whatever water might be here
    enviro.is_slippy = !(water._is_water && enviro.inwater) && (0 != mesh->test_fx(pchr->getTile(), MAPFX_SLIPPY));

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
            if(!enviro.is_slippy || g_meshLookupTables.twist_flat[mesh->get_twist(pchr->getTile())]) {
                pchr->jumpnumber = pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);                
            }
        }
    }
}
