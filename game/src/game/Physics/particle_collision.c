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
#include "game/ObjectPhysics.h"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/char.h"
#include "game/physics.h"
#include "egolib/Logic/Action.hpp"
#include "game/Entities/_Include.hpp"
#include "game/Module/Module.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//Constants
static constexpr float MAX_KNOCKBACK_VELOCITY = 40.0f;
static constexpr float DEFAULT_KNOCKBACK_VELOCITY = 10.0f;
static constexpr size_t COLLISION_LIST_SIZE = 256;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// data block used to communicate between the different "modules" governing the character-particle collision
struct chr_prt_collision_data_t
{
public:
    chr_prt_collision_data_t();

public:
    // object parameters
    CHR_REF ichr;
    Object *pchr;

    PRT_REF iprt;
    std::shared_ptr<Ego::Particle> pprt;
    std::shared_ptr<pip_t> ppip;

    //---- collision parameters

    // true collisions
    bool int_min;
    float depth_min;

    // hit-box collisions
    bool int_max;
    float depth_max;

    bool is_impact;
    bool is_pressure;
    bool is_collision;
    float dot;
    Vector3f nrm;

    // collision modifications
    bool mana_paid;
    int max_damage, actual_damage;
    Vector3f vdiff, vdiff_para, vdiff_perp;
    float block_factor;

    // collision reaction
    bool terminate_particle;
    bool prt_bumps_chr;
    bool prt_damages_chr;

    static void init(chr_prt_collision_data_t& self);
};

static bool do_chr_prt_collision_deflect(chr_prt_collision_data_t * pdata);
static bool do_chr_prt_collision_damage(chr_prt_collision_data_t * pdata);
static bool do_chr_prt_collision_bump(chr_prt_collision_data_t * pdata);
static bool do_chr_prt_collision_handle_bump(chr_prt_collision_data_t * pdata);

chr_prt_collision_data_t::chr_prt_collision_data_t() :
    ichr(INVALID_CHR_REF),
    pchr(nullptr),

    iprt(INVALID_PRT_REF),
    pprt(nullptr),
    ppip(nullptr),

    int_min(false),
    depth_min(0.0f),
    int_max(false),
    depth_max(0.0f),

    is_impact(false),
    is_pressure(false),
    is_collision(false),
    dot(0.0f),
    nrm(),

    mana_paid(false),
    max_damage(0),
    actual_damage(0),
    vdiff(), 
    vdiff_para(), 
    vdiff_perp(),
    block_factor(0.0f),

    terminate_particle(false),
    prt_bumps_chr(false),
    prt_damages_chr(false)
{
    //ctor
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool do_chr_prt_collision_init( const CHR_REF ichr, const PRT_REF iprt, chr_prt_collision_data_t * pdata );
static bool do_chr_prt_collision_get_details( chr_prt_collision_data_t * pdata, const float tmin, const float tmax );

static bool attach_prt_to_platform( Ego::Particle * pprt, Object * pplat );

//--------------------------------------------------------------------------------------------
bool get_prt_mass( Ego::Particle * pprt, Object * pchr, float * wt )
{
    /// @author BB
    /// @details calculate a "mass" for each object, taking into account possible infinite masses.

    float loc_wprt;

    if ( NULL == pprt || NULL == pchr ) return false;

    if ( NULL == wt ) wt = &loc_wprt;

    // determine an approximate mass for the particle
    if ( 0.0f == pprt->phys.bumpdampen )
    {
        *wt = -( float )CHR_INFINITE_WEIGHT;
    }
    else if ( pprt->isAttached() )
    {
        if ( CHR_INFINITE_WEIGHT == pprt->phys.weight || 0.0f == pprt->phys.bumpdampen )
        {
            *wt = -( float )CHR_INFINITE_WEIGHT;
        }
        else
        {
            *wt = pprt->phys.weight / pprt->phys.bumpdampen;
        }
    }
    else
    {
        float max_damage = std::abs(pprt->damage.base) + std::abs(pprt->damage.rand);

        *wt = 1.0f;

        if ( 0 == max_damage )
        {
            // this is a particle like the wind particles in the whirlwind
            // make the particle have some kind of predictable constant effect
            // relative to any character;
            *wt = pchr->phys.weight / 10.0f;
        }
        else
        {
            // determine an "effective mass" for the particle, based on it's max damage
            // and velocity

            float prt_vel2;
            float prt_ke;
            Vector3f vdiff;

            vdiff = pprt->vel - pchr->vel;

            // the damage is basically like the kinetic energy of the particle
            prt_vel2 = vdiff.dot(vdiff);

            // It can happen that a damage particle can hit something
            // at almost zero velocity, which would make for a huge "effective mass".
            // by making a reasonable "minimum velocity", we limit the maximum mass to
            // something reasonable
            prt_vel2 = std::max( 100.0f, prt_vel2 );

            // get the "kinetic energy" from the damage
            prt_ke = 3.0f * max_damage;

            // the faster the particle is going, the smaller the "mass" it
            // needs to do the damage
            *wt = prt_ke / ( 0.5f * prt_vel2 );
        }

        *wt /= pprt->phys.bumpdampen;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void get_recoil_factors( float wta, float wtb, float * recoil_a, float * recoil_b )
{
    float loc_recoil_a, loc_recoil_b;

    if ( NULL == recoil_a ) recoil_a = &loc_recoil_a;
    if ( NULL == recoil_b ) recoil_b = &loc_recoil_b;

    if ( wta >= ( float )CHR_INFINITE_WEIGHT ) wta = -( float )CHR_INFINITE_WEIGHT;
    if ( wtb >= ( float )CHR_INFINITE_WEIGHT ) wtb = -( float )CHR_INFINITE_WEIGHT;

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

//--------------------------------------------------------------------------------------------
bool do_prt_platform_detection( const CHR_REF ichr_a, const PRT_REF iprt_b )
{
    Object * pchr_a;

    bool platform_a;

    oct_vec_v2_t odepth;
    bool collide_x  = false;
    bool collide_y  = false;
    bool collide_xy = false;
    bool collide_yx = false;
    bool collide_z  = false;

    // make sure that A is valid
    if ( !_currentModule->getObjectHandler().exists( ichr_a ) ) return false;
    pchr_a = _currentModule->getObjectHandler().get( ichr_a );

    // make sure that B is valid
    const std::shared_ptr<Ego::Particle> &pprt_b = ParticleHandler::get()[iprt_b];
    if ( !pprt_b || pprt_b->isTerminated() ) return false;

    //Already attached to a platform?
    if(!_currentModule->getObjectHandler().exists(pprt_b->onwhichplatform_ref)) {
        return false;
    }

    // if you are mounted, only your mount is affected by platforms
    if ( _currentModule->getObjectHandler().exists( pchr_a->attachedto ) || pprt_b->isAttached() ) return false;

    // only check possible object-platform interactions
    platform_a = /* pprt_b->canuseplatforms && */ pchr_a->platform;
    if ( !platform_a ) return false;

    odepth[OCT_Z]  = std::min( pprt_b->prt_max_cv._maxs[OCT_Z] + pprt_b->pos[kZ], pchr_a->chr_min_cv._maxs[OCT_Z] + pchr_a->getPosZ() ) -
                     std::max( pprt_b->prt_max_cv._mins[OCT_Z] + pprt_b->pos[kZ], pchr_a->chr_min_cv._mins[OCT_Z] + pchr_a->getPosZ() );

    collide_z = TO_C_BOOL(odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE);

    if ( !collide_z ) return false;

    // determine how the characters can be attached
    odepth[OCT_Z] = ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] ) - ( pprt_b->pos[kZ] + pprt_b->prt_max_cv._mins[OCT_Z] );

    // size of b doesn't matter

    odepth[OCT_X] = std::min((pchr_a->chr_min_cv._maxs[OCT_X] + pchr_a->getPosX()) - pprt_b->pos[kX],
                              pprt_b->pos[kX] - ( pchr_a->chr_min_cv._mins[OCT_X] + pchr_a->getPosX() ) );

    odepth[OCT_Y]  = std::min(( pchr_a->chr_min_cv._maxs[OCT_Y] + pchr_a->getPosY() ) -  pprt_b->pos[kY],
                                pprt_b->pos[kY] - ( pchr_a->chr_min_cv._mins[OCT_Y] + pchr_a->getPosY() ) );

    odepth[OCT_XY] = std::min(( pchr_a->chr_min_cv._maxs[OCT_XY] + ( pchr_a->getPosX() + pchr_a->getPosY() ) ) - ( pprt_b->pos[kX] + pprt_b->pos[kY] ),
                              ( pprt_b->pos[kX] + pprt_b->pos[kY] ) - ( pchr_a->chr_min_cv._mins[OCT_XY] + ( pchr_a->getPosX() + pchr_a->getPosY() ) ) );

    odepth[OCT_YX] = std::min(( pchr_a->chr_min_cv._maxs[OCT_YX] + ( -pchr_a->getPosX() + pchr_a->getPosY() ) ) - ( -pprt_b->pos[kX] + pprt_b->pos[kY] ),
                              ( -pprt_b->pos[kX] + pprt_b->pos[kY] ) - ( pchr_a->chr_min_cv._mins[OCT_YX] + ( -pchr_a->getPosX() + pchr_a->getPosY() ) ) );

    collide_x  = TO_C_BOOL( odepth[OCT_X]  > 0.0f );
    collide_y  = TO_C_BOOL( odepth[OCT_Y]  > 0.0f );
    collide_xy = TO_C_BOOL( odepth[OCT_XY] > 0.0f );
    collide_yx = TO_C_BOOL( odepth[OCT_YX] > 0.0f );
    collide_z  = TO_C_BOOL( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( collide_x && collide_y && collide_xy && collide_yx && collide_z )
    {
        // check for the best possible attachment
        if ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] > pprt_b->targetplatform_level )
        {
            pprt_b->targetplatform_level = pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z];
            pprt_b->targetplatform_ref   = ichr_a;

            attach_prt_to_platform(pprt_b.get(), pchr_a);
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------

bool do_chr_prt_collision_get_details(chr_prt_collision_data_t * pdata, const float tmin, const float tmax)
{
    // Get details about the character-particle interaction
    //
    // We already know that the largest particle cv intersects with the a
    // character cv sometime this frame. We need more details to know
    // how to handle the collision.

    bool handled;

    float exponent;
    oct_bb_t cv_chr, cv_prt_max, cv_prt_min;
    oct_vec_v2_t odepth;

    if ( NULL == pdata ) return false;

    // make the object more like a table if there is a platform-like interaction
    exponent = 1;
    if ( SPRITE_SOLID == pdata->pprt->type && pdata->pchr->platform ) exponent += 2;

    // assume the simplest interaction normal
    pdata->nrm = Vector3f(0, 0, 1);

    // no valid interactions, yet
    handled = false;

    // shift the source bounding boxes to be centered on the given positions
    oct_bb_t::translate(pdata->pchr->chr_min_cv, pdata->pchr->getPosition(), cv_chr);

    // the smallest particle collision volume
    oct_bb_t::translate(pdata->pprt->prt_min_cv, pdata->pprt->getPosition(), cv_prt_min);

    // the largest particle collision volume (the hit-box)
    oct_bb_t::translate(pdata->pprt->prt_max_cv, pdata->pprt->getPosition(), cv_prt_max);

    if ( tmin <= 0.0f || std::abs( tmin ) > 1e6 || std::abs( tmax ) > 1e6 )
    {
        // use "pressure" to determine the normal and overlap
        phys_estimate_pressure_normal(cv_prt_min, cv_chr, exponent, odepth, pdata->nrm, pdata->depth_min);

        handled = true;
        if ( tmin <= 0.0f )
        {
            handled = TO_C_BOOL( pdata->depth_min > 0.0f );
        }

        // tag the type of interaction
        pdata->int_min = handled;
        pdata->is_pressure = handled;
    }
    else
    {
        // find the collision volumes at 10% overlap
        oct_bb_t exp1, exp2;

        float tmp_min, tmp_max;

        tmp_min = tmin;
        tmp_max = tmin + ( tmax - tmin ) * 0.1f;

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb(cv_prt_min, pdata->pprt->vel, tmp_min, tmp_max, exp1);
        phys_expand_oct_bb(cv_chr,     pdata->pchr->vel, tmp_min, tmp_max, exp2);

        // use "collision" to determine the normal and overlap
        handled = phys_estimate_collision_normal(exp1, exp2, exponent, odepth, pdata->nrm, pdata->depth_min);

        // tag the type of interaction
        pdata->int_min      = handled;
        pdata->is_collision = handled;
    }

    if ( !handled )
    {
        if ( tmin <= 0.0f || std::abs( tmin ) > 1e6 || std::abs( tmax ) > 1e6 )
        {
            // use "pressure" to determine the normal and overlap
            phys_estimate_pressure_normal(cv_prt_max, cv_chr, exponent, odepth, pdata->nrm, pdata->depth_max);

            handled = true;
            if ( tmin <= 0.0f )
            {
                handled = TO_C_BOOL( pdata->depth_max > 0.0f );
            }

            // tag the type of interaction
            pdata->int_max     = handled;
            pdata->is_pressure = handled;
        }
        else
        {
            // find the collision volumes at 10% overlap
            oct_bb_t exp1, exp2;

            float tmp_min, tmp_max;

            tmp_min = tmin;
            tmp_max = tmin + ( tmax - tmin ) * 0.1f;

            // determine the expanded collision volumes for both objects
            phys_expand_oct_bb(cv_prt_max, pdata->pprt->vel, tmp_min, tmp_max, exp1);
            phys_expand_oct_bb(cv_chr,     pdata->pchr->vel, tmp_min, tmp_max, exp2);

            // use "collision" to determine the normal and overlap
            handled = phys_estimate_collision_normal(exp1, exp2, exponent, odepth, pdata->nrm, pdata->depth_max);

            // tag the type of interaction
            pdata->int_max      = handled;
            pdata->is_collision = handled;
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_deflect(chr_prt_collision_data_t * pdata)
{
    bool prt_deflected = false;

    /// @note ZF@> Simply ignore characters with invictus for now, it causes some strange effects
    if ( pdata->pchr->isInvincible() ) return true;

    //Don't deflect money or particles spawned by the Object itself
    bool prt_wants_deflection = (pdata->pprt->owner_ref != pdata->pchr->getCharacterID()) && !pdata->ppip->bump_money;
    if(!prt_wants_deflection) {
        return false;
    }

    // find the "attack direction" of the particle
    FACING_T direction = vec_to_facing(pdata->pchr->getPosX() - pdata->pprt->getPosX(), pdata->pchr->getPosY() - pdata->pprt->getPosY());
    direction = pdata->pchr->ori.facing_z - direction + ATK_BEHIND;

    // shield block?
    bool chr_is_invictus = pdata->pchr->isInvictusDirection(direction, pdata->ppip->damfx);

    // try to deflect the particle
    bool chr_can_deflect = (0 != pdata->pchr->damage_timer) && (pdata->max_damage > 0);
    prt_deflected = false;
    pdata->mana_paid = false;
    if(chr_can_deflect)
    {
        MissileTreatmentType treatment = MISSILE_NORMAL;

        // make a ricochet if the character is invictus
        if(chr_is_invictus) {
            treatment = MISSILE_DEFLECT;
            prt_deflected = true;
        }

        //Check if the target has any enchantment that can deflect missiles
        else {
            for(const std::shared_ptr<Ego::Enchantment> &enchant : pdata->pchr->getActiveEnchants()) {
                if(enchant->isTerminated()) continue;

                //Does this enchant provide special missile protection?
                if(enchant->getMissileTreatment() != MISSILE_NORMAL) {
                    if(enchant->getOwner() != nullptr) {
                        if(enchant->getOwner()->costMana(enchant->getMissileTreatmentCost(), pdata->pprt->owner_ref)) {
                            pdata->mana_paid = true;
                            treatment = enchant->getMissileTreatment();
                            prt_deflected = true;
                            break;
                        }
                    }
                }
            }
        }

        //Was it deflected somehow?
        if (prt_deflected)
        {
            // Treat the missile
            if ( treatment == MISSILE_DEFLECT )
            {
                // Deflect the incoming ray off the normal
                pdata->pprt->phys.avel -= pdata->vdiff_para * 2.0f;

                // the ricochet is not guided
                pdata->pprt->setHoming(false);
            }
            else if ( treatment == MISSILE_REFLECT )
            {
                // Reflect it back in the direction it came
                pdata->pprt->phys.avel -= pdata->vdiff * 2.0f;

                // Change the owner of the missile
                pdata->pprt->team       = pdata->pchr->team;
                pdata->pprt->owner_ref  = pdata->pchr->getCharacterID();
            }
        }
    }

    if (chr_is_invictus || prt_deflected)
    {
        bool using_shield = false;

        // If the attack was blocked by a shield, then check if the block caused a knockback
        if ( chr_is_invictus && ACTION_IS_TYPE(pdata->pchr->inst.action_which, P) )
        {
            // Figure out if we are really using a shield or if it is just a invictus frame
            CHR_REF item = INVALID_CHR_REF;

            // Check right hand for a shield
            if ( !using_shield )
            {
                item = pdata->pchr->holdingwhich[SLOT_RIGHT];
                if ( _currentModule->getObjectHandler().exists( item ) && pdata->pchr->ai.lastitemused == item )
                {
                    using_shield = true;
                }
            }

            // Check left hand for a shield
            if ( !using_shield )
            {
                item = pdata->pchr->holdingwhich[SLOT_LEFT];
                if ( _currentModule->getObjectHandler().exists( item ) && pdata->pchr->ai.lastitemused == item )
                {
                    using_shield = true;
                }
            }

            // Now we have the block rating and know the enemy
            if ( _currentModule->getObjectHandler().exists( pdata->pprt->owner_ref ) && using_shield )
            {
                int   total_block_rating;

                Object *pshield   = _currentModule->getObjectHandler().get( item );
                Object *pattacker = _currentModule->getObjectHandler().get( pdata->pprt->owner_ref );

                // use the character block skill plus the base block rating of the shield and adjust for strength
                total_block_rating = pshield->getProfile()->getBaseBlockRating();

                //Defender Perk gives +100% Block Rating
                if(pdata->pchr->hasPerk(Ego::Perks::DEFENDER)) {
                    total_block_rating += 100;
                }

                // -4% per attacker strength
                total_block_rating -= 4 * pattacker->getAttribute(Ego::Attribute::MIGHT);

                // +2% per defender strength
                total_block_rating += 2 * pdata->pchr->getAttribute(Ego::Attribute::MIGHT);

                // Now determine the result of the block
                if ( Random::getPercent() <= total_block_rating )
                {
                    // Defender won, the block holds
                    // Add a small stun to the attacker = 40/50 (0.8 seconds)
                    pattacker->reload_timer += 40;
                }
                else
                {
                    // Attacker broke the block and batters away the shield
                    // Time to raise shield again = 40/50 (0.8 seconds)
                    pdata->pchr->reload_timer += 40;
                    AudioSystem::get().playSound(pdata->pchr->getPosition(), AudioSystem::get().getGlobalSound(GSND_SHIELDBLOCK));
                }
            }
        }

        // Tell the players that the attack was somehow deflected
        if(0 == pdata->pchr->damage_timer) 
        {
            ParticleHandler::get().spawnDefencePing(pdata->pchr->toSharedPointer(), _currentModule->getObjectHandler()[pdata->pprt->owner_ref]);
            if(using_shield) {
                chr_make_text_billboard(pdata->pchr->getCharacterID(), "Blocked!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f(getBlockActionColour(), 1.0f), 3, Billboard::Flags::All);
            }
            else {
                chr_make_text_billboard(pdata->pchr->getCharacterID(), "Deflected!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f(getBlockActionColour(), 1.0f), 3, Billboard::Flags::All);
            }
        }
    }

    return prt_deflected;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_damage( chr_prt_collision_data_t * pdata )
{
    Object * powner = NULL;

    if ( NULL == pdata ) return false;

    if ( _currentModule->getObjectHandler().exists( pdata->pprt->owner_ref ) )
    {
        powner = _currentModule->getObjectHandler().get( pdata->pprt->owner_ref );
    }

    //Get the Profile of the Object that spawned this particle (i.e the weapon itself, not the holder)
    const std::shared_ptr<ObjectProfile> &spawnerProfile = ProfileSystem::get().getProfile(pdata->pprt->getSpawnerProfile());
    if(spawnerProfile != nullptr) { //global particles do not have a spawner profile, so this is possible
        // Check all enchants to see if they are removed
        for(const std::shared_ptr<Ego::Enchantment> &enchant : pdata->pchr->getActiveEnchants()) {
            if(enchant->isTerminated()) {
                continue;
            }

            // if nothing can remove it, just go on with your business
            if(enchant->getProfile()->removedByIDSZ == IDSZ_NONE) {
                continue;
            }

            // check vs. every IDSZ that could have something to do with cancelling the enchant
            if ( enchant->getProfile()->removedByIDSZ == spawnerProfile->getIDSZ(IDSZ_TYPE) ||
                 enchant->getProfile()->removedByIDSZ == spawnerProfile->getIDSZ(IDSZ_PARENT) ) {
                enchant->requestTerminate();
            }
        }        
    }

    // Steal some life.
    if ( pdata->pprt->lifedrain > 0 && pdata->pchr->getLife() > 0)
    {
        // Drain as much as allowed and possible.
        float drain = std::min(pdata->pchr->getLife(), FP8_TO_FLOAT(pdata->pprt->lifedrain));

        // Remove the drain from the character that was hit ...
        pdata->pchr->setLife(pdata->pchr->getLife() - drain);

        // ... and add it to the "caster".
        if ( NULL != powner )
        {
            powner->setLife(powner->getLife() + drain);
        }
    }

    // Steal some mana.
    if ( pdata->pprt->manadrain > 0 && pdata->pchr->getMana() > 0)
    {
        // Drain as much as allowed and possible.
        float drain = std::min(pdata->pchr->getMana(), FP8_TO_FLOAT(pdata->pprt->manadrain));

        // Remove the drain from the character that was hit ...
        pdata->pchr->setMana(pdata->pchr->getMana() - drain);

        // add it to the "caster"
        if ( NULL != powner )
        {
            powner->setMana(powner->getMana() + drain);
        }
    }

    // Do grog
    if (pdata->ppip->grogTime > 0 && pdata->pchr->getProfile()->canBeGrogged())
    {
        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
        pdata->pchr->grog_timer = std::max(static_cast<unsigned>(pdata->pchr->grog_timer), pdata->ppip->grogTime );

        chr_make_text_billboard(powner->getCharacterID(), "Groggy!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::green(), 3, Billboard::Flags::All);
    }

    // Do daze
    if (pdata->ppip->dazeTime > 0 && pdata->pchr->getProfile()->canBeDazed())
    {
        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
        pdata->pchr->daze_timer = std::max(static_cast<unsigned>(pdata->pchr->daze_timer), pdata->ppip->dazeTime );

        chr_make_text_billboard(powner->getCharacterID(), "Dazed!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::yellow(), 3, Billboard::Flags::All);
    }

    //---- Damage the character, if necessary
    if ( 0 != std::abs( pdata->pprt->damage.base ) + std::abs( pdata->pprt->damage.rand ) )
    {
        //bool prt_needs_impact = TO_C_BOOL( pdata->ppip->rotatetoface || pdata->pprt->isAttached() );
        //if(spawnerProfile != nullptr) {
        //    if ( spawnerProfile->isRangedWeapon() ) prt_needs_impact = true;            
        //}

        // DAMFX_ARRO means that it only does damage to the one it's attached to
        if ( HAS_NO_BITS(pdata->ppip->damfx, DAMFX_ARRO) /*&& (!prt_needs_impact || pdata->is_impact)*/ )
        {
            //Damage adjusted for attributes and weaknesses
            IPair modifiedDamage = pdata->pprt->damage;

            FACING_T direction = vec_to_facing( pdata->pprt->vel[kX] , pdata->pprt->vel[kY] );
            direction = pdata->pchr->ori.facing_z - direction + ATK_BEHIND;

            // These things only apply if the particle has an owner
            if ( nullptr != powner )
            {
                //Check special perk effects
                if(spawnerProfile != nullptr)
                {                
                    // Check Crack Shot perk which applies 3 second Daze with fireweapons
                    if(pdata->pchr->getProfile()->canBeDazed() && powner->hasPerk(Ego::Perks::CRACKSHOT) && DamageType_isPhysical(pdata->pprt->damagetype))
                    {
                        //Is the particle spawned by a gun?
                        if(spawnerProfile->isRangedWeapon() && spawnerProfile->getIDSZ(IDSZ_SKILL) == MAKE_IDSZ('T','E','C','H')) {
                            SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
                            pdata->pchr->daze_timer += 3;

                            chr_make_text_billboard(powner->getCharacterID(), "Crackshot!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
                        }
                    }

                    //Brutal Strike has chance to inflict 2 second Grog with melee CRUSH attacks
                    if(pdata->pchr->getProfile()->canBeGrogged() && powner->hasPerk(Ego::Perks::BRUTAL_STRIKE) && spawnerProfile->isMeleeWeapon() && pdata->pprt->damagetype == DAMAGE_CRUSH) {
                        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
                        pdata->pchr->grog_timer += 2;

                        chr_make_text_billboard(powner->getCharacterID(), "Brutal Strike!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::red(), 3, Billboard::Flags::All);
                        AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                    }
                }

                // Apply intellect bonus damage for particles with the [IDAM] expansions (Low ability gives penality)
                // +2% bonus for every point of intellect. Below 14 gives -2% instead!
                if ( pdata->ppip->_intellectDamageBonus )
                {
                    float percent = ( powner->getAttribute(Ego::Attribute::INTELLECT) - 14.0f ) * 2.0f;

                    //Sorcery Perk increases spell damage by 10%
                    if(powner->hasPerk(Ego::Perks::SORCERY)) {
                        percent += 10.0f;
                    }

                    //Dark Arts Master perk gives evil damage +20%
                    if(pdata->pprt->damagetype == DAMAGE_EVIL && powner->hasPerk(Ego::Perks::DARK_ARTS_MASTERY)) {
                        percent += 20.0f;
                    }

                    percent /= 100.0f;
                    modifiedDamage.base *= 1.00f + percent;
                    modifiedDamage.rand *= 1.00f + percent;

                    //Disintegrate perk deals +100 ZAP damage at 0.025% chance per Intellect!
                    if(pdata->pprt->damagetype == DAMAGE_ZAP && powner->hasPerk(Ego::Perks::DISINTEGRATE)) {
                        if(Random::nextFloat()*100.0f <= powner->getAttribute(Ego::Attribute::INTELLECT) * 0.025f) {
                            modifiedDamage.base += FLOAT_TO_FP8(100.0f);
                            chr_make_text_billboard(pdata->pchr->getCharacterID(), "Disintegrated!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::purple(), 6, Billboard::Flags::All);

                            //Disintegrate effect
                            ParticleHandler::get().spawnGlobalParticle(pdata->pchr->getPosition(), ATK_FRONT, LocalParticleProfileRef(PIP_DISINTEGRATE_START), 0);
                        }
                    }
                }

                // Notify the attacker of a scored hit
                SET_BIT(powner->ai.alert, ALERTIF_SCOREDAHIT);
                powner->ai.hitlast = pdata->pchr->getCharacterID();

                // Tell the weapons who the attacker hit last
                bool meleeAttack = false;
                const std::shared_ptr<Object> &leftHanditem = powner->getRightHandItem();
                if (leftHanditem)
                {
                    leftHanditem->ai.hitlast = pdata->pchr->getCharacterID();
                    if (powner->ai.lastitemused == leftHanditem->getCharacterID()) {
                        SET_BIT(leftHanditem->ai.alert, ALERTIF_SCOREDAHIT);  
                        if(leftHanditem->getProfile()->getIDSZ(IDSZ_SPECIAL) == MAKE_IDSZ('X', 'W', 'E', 'P') && !leftHanditem->getProfile()->isRangedWeapon()) {
                            meleeAttack = true;
                        }
                    } 
                }

                const std::shared_ptr<Object> &rightHandItem = powner->getRightHandItem();
                if (rightHandItem)
                {
                    rightHandItem->ai.hitlast = pdata->pchr->getCharacterID();
                    if (powner->ai.lastitemused == rightHandItem->getCharacterID()) {
                        SET_BIT(rightHandItem->ai.alert, ALERTIF_SCOREDAHIT);  
                        if(rightHandItem->getProfile()->getIDSZ(IDSZ_SPECIAL) == MAKE_IDSZ('X', 'W', 'E', 'P') && !rightHandItem->getProfile()->isRangedWeapon()) {
                            meleeAttack = true;
                        }
                    } 
                }

                //Unarmed attack?
                if (powner->ai.lastitemused == powner->getCharacterID()) {
                    meleeAttack = true;
                }

                //Grim Reaper (5% chance to trigger +50 EVIL damage)
                if(spawnerProfile != nullptr && powner->hasPerk(Ego::Perks::GRIM_REAPER)) {

                    //Is it a Scythe?
                    if(spawnerProfile->getIDSZ(IDSZ_TYPE) == MAKE_IDSZ('S','C','Y','T') && Random::getPercent() <= 5) {

                        //Make sure they can be damaged by EVIL first
                        if(pdata->pchr->getAttribute(Ego::Attribute::EVIL_MODIFIER) == NONE) {
                            IPair grimReaperDamage;
                            grimReaperDamage.base = FLOAT_TO_FP8(50.0f);
                            grimReaperDamage.rand = 0.0f;
                            pdata->pchr->damage(direction, grimReaperDamage, DAMAGE_EVIL, pdata->pprt->team, _currentModule->getObjectHandler()[pdata->pprt->owner_ref], DAMFX_TIME, false);
                            chr_make_text_billboard(powner->getCharacterID(), "Grim Reaper!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::red(), 3, Billboard::Flags::All);                                
                            AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                        }
                    }
                }                

                //Deadly Strike perk (1% chance per character level to trigger vs non undead)
                if(meleeAttack && pdata->pchr->getProfile()->getIDSZ(IDSZ_PARENT) != MAKE_IDSZ('U','N','D','E'))
                {
                    if(powner->hasPerk(Ego::Perks::DEADLY_STRIKE) && powner->getExperienceLevel() >= Random::getPercent() && DamageType_isPhysical(pdata->pprt->damagetype)){
                        //Gain +0.25 damage per Agility
                        modifiedDamage.base += FLOAT_TO_FP8(powner->getAttribute(Ego::Attribute::AGILITY) * 0.25f);
                        chr_make_text_billboard(powner->getCharacterID(), "Deadly Strike", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
                        AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                    }
                }
            }

            // handle vulnerabilities, double the damage
            if (pdata->pchr->getProfile()->getIDSZ(IDSZ_VULNERABILITY) == spawnerProfile->getIDSZ(IDSZ_TYPE) || 
                pdata->pchr->getProfile()->getIDSZ(IDSZ_VULNERABILITY) == spawnerProfile->getIDSZ(IDSZ_PARENT))
            {
                // Double the damage
                modifiedDamage.base = ( modifiedDamage.base << 1 );
                modifiedDamage.rand = ( modifiedDamage.rand << 1 ) | 1;

                SET_BIT( pdata->pchr->ai.alert, ALERTIF_HITVULNERABLE );

                // Initialize for the billboard
                chr_make_text_billboard(pdata->pchr->getCharacterID(), "Super Effective!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::yellow(), 3, Billboard::Flags::All);
            }

            //Is it a critical hit?
            if(powner && powner->hasPerk(Ego::Perks::CRITICAL_HIT) && DamageType_isPhysical(pdata->pprt->damagetype)) {
                //0.5% chance per agility to deal max damage
                float critChance = powner->getAttribute(Ego::Attribute::AGILITY)*0.5f;

                //Lucky Perk increases critical hit chance by 10%!
                if(powner->hasPerk(Ego::Perks::LUCKY)) {
                    critChance += 10.0f;
                }

                if(Random::getPercent() <= critChance) {
                    modifiedDamage.base += modifiedDamage.rand;
                    modifiedDamage.rand = 0;
                    chr_make_text_billboard(powner->getCharacterID(), "Critical Hit!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::red(), 3, Billboard::Flags::All);
                    AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                }
            }

            // Damage the character
            pdata->actual_damage = pdata->pchr->damage(direction, modifiedDamage, pdata->pprt->damagetype, 
                pdata->pprt->team, _currentModule->getObjectHandler()[pdata->pprt->owner_ref], pdata->ppip->damfx, false);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_bump( chr_prt_collision_data_t * pdata )
{
    bool prt_belongs_to_chr;
    bool prt_hates_chr, prt_attacks_chr;
    bool valid_onlydamagefriendly;
    bool valid_friendlyfire;
    bool valid_onlydamagehate;

    if ( NULL == pdata ) return false;

    const float maxDamage = std::abs(pdata->pprt->damage.base) + std::abs(pdata->pprt->damage.rand);

    // always allow valid reaffirmation
    if (( pdata->pchr->reaffirm_damagetype < DAMAGE_COUNT ) &&
        ( pdata->pprt->damagetype < DAMAGE_COUNT ) &&
        ( pdata->pchr->reaffirm_damagetype == pdata->pprt->damagetype ) &&
        ( maxDamage > 0) )
    {
        return true;
    }

    //Only allow one collision per particle unless that particle is eternal
    if(!pdata->pprt->isEternal() && pdata->pprt->hasCollided(_currentModule->getObjectHandler()[pdata->pchr->getCharacterID()])) {
        return false;
    }

    prt_belongs_to_chr = TO_C_BOOL(pdata->pchr->getCharacterID() == pdata->pprt->owner_ref);

    if ( !prt_belongs_to_chr )
    {
        // no simple owner relationship. Check for something deeper.
        CHR_REF prt_owner = prt_get_iowner( pdata->pprt->getParticleID(), 0 );
        if ( _currentModule->getObjectHandler().exists( prt_owner ) )
        {
            CHR_REF chr_wielder = chr_get_lowest_attachment( pdata->pchr->getCharacterID(), true );
            CHR_REF prt_wielder = chr_get_lowest_attachment( prt_owner, true );

            if ( !_currentModule->getObjectHandler().exists( chr_wielder ) ) chr_wielder = pdata->pchr->getCharacterID();
            if ( !_currentModule->getObjectHandler().exists( prt_wielder ) ) prt_wielder = prt_owner;

            prt_belongs_to_chr = TO_C_BOOL(chr_wielder == prt_wielder);
        }
    }

    // does the particle team hate the character's team
    prt_hates_chr = team_hates_team( pdata->pprt->team, pdata->pchr->team );

    // Only bump into hated characters?
    valid_onlydamagehate = TO_C_BOOL( prt_hates_chr && pdata->pprt->getProfile()->hateonly );

    // allow neutral particles to attack anything
    prt_attacks_chr = false;
    if(prt_hates_chr || ((Team::TEAM_NULL != pdata->pchr->team) && (Team::TEAM_NULL == pdata->pprt->team)) ) {
        prt_attacks_chr = (maxDamage > 0);
    }

    // this is the onlydamagefriendly condition from the particle search code
    valid_onlydamagefriendly = TO_C_BOOL(( pdata->ppip->onlydamagefriendly && pdata->pprt->team == pdata->pchr->team ) ||
                                         ( !pdata->ppip->onlydamagefriendly && prt_attacks_chr ) );

    // I guess "friendly fire" does not mean "self fire", which is a bit unfortunate.
    valid_friendlyfire = TO_C_BOOL(( pdata->ppip->friendlyfire && !prt_hates_chr && !prt_belongs_to_chr ) ||
                                   ( !pdata->ppip->friendlyfire && prt_attacks_chr ) );

    pdata->prt_bumps_chr =  TO_C_BOOL( valid_friendlyfire || valid_onlydamagefriendly || valid_onlydamagehate );

    return pdata->prt_bumps_chr;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_handle_bump( chr_prt_collision_data_t * pdata )
{
    if ( NULL == pdata || !pdata->prt_bumps_chr ) return false;

    if ( !pdata->prt_bumps_chr ) return false;

    // Catch on fire
    spawn_bump_particles( pdata->pchr->getCharacterID(), pdata->pprt->getParticleID() );

    // handle some special particle interactions
    if ( pdata->pprt->getProfile()->end_bump )
    {
        if (pdata->pprt->getProfile()->bump_money)
        {
            Object * pcollector = pdata->pchr;

            // Let mounts collect money for their riders
            if (pdata->pchr->isMount())
            {
                // if the mount's rider can't get money, the mount gets to keep the money!
                const std::shared_ptr<Object> &rider = pdata->pchr->getLeftHandItem();
                if (rider != nullptr && rider->getProfile()->canGrabMoney()) {
                    pcollector = rider.get();
                }
            }

            if ( pcollector->getProfile()->canGrabMoney() && pcollector->isAlive() && 0 == pcollector->damage_timer && pcollector->money < MAXMONEY )
            {
                pcollector->money += pdata->pprt->getProfile()->bump_money;
                pcollector->money = Ego::Math::constrain<int>(pcollector->money, 0, MAXMONEY);

                // the coin disappears when you pick it up
                pdata->terminate_particle = true;
            }
        }
        else
        {
            // Only hit one character, not several
            pdata->terminate_particle = true;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_init( const CHR_REF ichr, const PRT_REF iprt, chr_prt_collision_data_t * pdata )
{
    if ( NULL == pdata ) return false;

    BLANK_STRUCT_PTR(pdata);

    if ( !ParticleHandler::get()[iprt] ) return false;
    pdata->iprt = iprt;
    pdata->pprt = ParticleHandler::get()[iprt];

    // make sure that it is on
    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return false;
    pdata->ichr = ichr;
    pdata->pchr = _currentModule->getObjectHandler().get( ichr );

    pdata->ppip = pdata->pprt->getProfile();

    // estimate the maximum possible "damage" from this particle
    // other effects can magnify this number, like vulnerabilities
    // or DAMFX_* bits
    pdata->max_damage = std::abs( pdata->pprt->damage.base ) + std::abs( pdata->pprt->damage.rand );

    return true;
}

void do_chr_prt_collision_knockback(chr_prt_collision_data_t &pdata)
{
    /**
    * @brief
    *   ZF> New particle collision knockback algorithm (07.08.2015)
    **/

    //No knocback applicable?
    if(pdata.pprt->vel.length_abs() == 0.0f) {
        return;
    }

    //Target immune to knockback?
    if(pdata.pchr->phys.bumpdampen == 0.0f || CHR_INFINITE_WEIGHT == pdata.pchr->phys.weight) {
        return;
    }

    //Is particle allowed to cause knockback?
    if(!pdata.ppip->allowpush) {
        return;
    }

    //If the particle was magically deflected, then there is no knockback
    if (pdata.mana_paid) {
        return;
    }

    float knockbackFactor = 1.0f;

    //If we are attached to a Object then the attacker's Might can increase knockback
    std::shared_ptr<Object> attacker = pdata.pprt->getAttachedObject();
    if (attacker)
    {
        //If we are actually a weapon, use the weapon holder's strength
        if(attacker->isBeingHeld()) {
            attacker = _currentModule->getObjectHandler()[attacker->attachedto];
        }

        const float attackerMight = attacker->getAttribute(Ego::Attribute::MIGHT) - 10.0f;

        //Add 2% knockback per point of Might above 10
        if(attackerMight >= 0.0f) {
            knockbackFactor += attackerMight * 0.02f;
        }

        //Reduce knockback by 10% per point of Might below 10
        else {
            knockbackFactor += attackerMight * 0.1f;
        }

        //Telekinetic Staff perk can give +500% knockback
        const std::shared_ptr<Object>& powner = _currentModule->getObjectHandler()[pdata.pprt->owner_ref];
        if(powner->hasPerk(Ego::Perks::TELEKINETIC_STAFF) && 
            pdata.pprt->getAttachedObject()->getProfile()->getIDSZ(IDSZ_PARENT) == MAKE_IDSZ('S','T','A','F')) {

            //+3% chance per owner Intellect and -1% per target Might
            float chance = attacker->getAttribute(Ego::Attribute::INTELLECT) * 0.03f - pdata.pchr->getAttribute(Ego::Attribute::MIGHT)*0.01f;
            if(Random::nextFloat() <= chance) {
                knockbackFactor += 5.0f;
                chr_make_text_billboard(attacker->getCharacterID(), "Telekinetic Staff!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::purple(), 2, Billboard::Flags::All);
            }
        }
    }

    //Adjust knockback based on relative mass between particle and target
    if(pdata.pchr->phys.bumpdampen != 0.0f && CHR_INFINITE_WEIGHT != pdata.pchr->phys.weight) {
        float particleMass = 0.0f;
        float targetMass = get_chr_mass(pdata.pchr);
        get_prt_mass(pdata.pprt.get(), pdata.pchr, &particleMass);
        if(targetMass >= 0.0f) {
            knockbackFactor *= Ego::Math::constrain(particleMass / targetMass, 0.0f, 1.0f);
        }
    }

    //Amount of knockback is affected by damage type
    switch(pdata.pprt->damagetype)
    {
        // very blunt type of attack, the maximum effect
        case DAMAGE_CRUSH:
            knockbackFactor *= 1.0f;
        break;

        // very focussed type of attack, the minimum effect
        case DAMAGE_POKE:
            knockbackFactor *= 0.5f;
        break;        

        // all other damage types are in the middle
        default:
            knockbackFactor *= Ego::Math::invSqrtTwo<float>();
        break;
    }

    //Apply knockback to the victim (limit between 0% and 300% knockback)
    Vector3f knockbackVelocity = pdata.pprt->vel * Ego::Math::constrain(knockbackFactor, 0.0f, 3.0f);
    //knockbackVelocity[kX] = std::cos(pdata.pprt->vel[kX]) * DEFAULT_KNOCKBACK_VELOCITY;
    //knockbackVelocity[kY] = std::sin(pdata.pprt->vel[kY]) * DEFAULT_KNOCKBACK_VELOCITY;
    //knockbackVelocity[kZ] = DEFAULT_KNOCKBACK_VELOCITY / 2;
    //knockbackVelocity *= Ego::Math::constrain(knockbackFactor, 0.0f, 3.0f);

    //Limit total horizontal knockback velocity to MAXTHROWVELOCITY
    const float magnitudeVelocityXY = std::sqrt(knockbackVelocity[kX]*knockbackVelocity[kX] + knockbackVelocity[kY]*knockbackVelocity[kY]);
    if(magnitudeVelocityXY > MAX_KNOCKBACK_VELOCITY) {
        knockbackVelocity[kX] *= MAX_KNOCKBACK_VELOCITY / magnitudeVelocityXY;
        knockbackVelocity[kY] *= MAX_KNOCKBACK_VELOCITY / magnitudeVelocityXY;
    }

    //Limit total vertical knockback velocity to one third of MAXTHROWVELOCTIY
    const float magnitudeVelocityZ = std::sqrt(knockbackVelocity[kZ]*knockbackVelocity[kZ]);
    if(magnitudeVelocityZ > MAX_KNOCKBACK_VELOCITY) {
        knockbackVelocity[kZ] *= MAX_KNOCKBACK_VELOCITY / magnitudeVelocityZ;
    }

    //Apply knockback
    pdata.pchr->phys.avel += knockbackVelocity;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision(const std::shared_ptr<Object> &object, const std::shared_ptr<Ego::Particle> &particle, const float tmin, const float tmax)
{
    /// @author BB
    /// @details this funciton goes through all of the steps to handle character-particle
    ///               interactions. A basic interaction has been detected. This needs to be refined
    ///               and then handled. The function returns false if the basic interaction was wrong
    ///               or if the interaction had no effect.
    ///
    /// @note This function is a little more complicated than the character-character case because
    ///       of the friend-foe logic as well as the damage and other special effects that particles can do.

    bool retval = false;

    chr_prt_collision_data_t cn_data;
    chr_prt_collision_data_t::init(cn_data);

    bool intialized = do_chr_prt_collision_init(object->getCharacterID(), particle->getParticleID(), &cn_data );
    if ( !intialized ) return false;

    // ignore dead characters
    if ( !cn_data.pchr->isAlive() ) return false;

    // skip objects that are inside inventories
    if ( cn_data.pchr->isInsideInventory() ) return false;

    // if the particle is attached to this character, ignore a "collision"
    if ( cn_data.pprt->getAttachedObject().get() ==  cn_data.pchr )
    {
        return false;
    }

    // is there any collision at all?
    if ( !do_chr_prt_collision_get_details(&cn_data, tmin, tmax) )
    {
        return false;
    }
    else
    {
        // help classify impacts

        if ( cn_data.is_pressure )
        {
            // on the odd chance that we want to use the pressure
            // algorithm for an obvious collision....
            if ( tmin > 0.0f ) cn_data.is_impact = true;

        }

        if ( cn_data.is_collision )
        {
            cn_data.is_impact = true;
        }
    }

    // if there is no collision, no point in going farther
    if (!cn_data.int_min && !cn_data.int_max) return false;

    // if the particle is not actually hitting the object, then limit the
    // interaction to 2d
    if (cn_data.int_max && !cn_data.int_min)
    {
        // do not re-normalize this vector
        cn_data.nrm[kZ] = 0.0f;
    }

    // find the relative velocity
    cn_data.vdiff = cn_data.pchr->vel - cn_data.pprt->vel;

    // decompose the relative velocity parallel and perpendicular to the surface normal
    cn_data.dot = fvec3_decompose(cn_data.vdiff, cn_data.nrm, cn_data.vdiff_perp, cn_data.vdiff_para);

    // determine whether the particle is deflected by the character
    bool prt_deflected = do_chr_prt_collision_deflect(&cn_data);
    if (prt_deflected) {
        retval = true;
    }

    // refine the logic for a particle to hit a character
    bool prt_can_hit_chr = !prt_deflected && do_chr_prt_collision_bump(&cn_data);

    // Torches and such are marked as invulnerable, so the particle is always deflected.
    // make a special case for reaffirmation
    if (0 == cn_data.pchr->damage_timer )
    {
        // Check reaffirmation of particles
        if ( cn_data.pchr->reaffirm_damagetype == cn_data.pprt->damagetype )
        {
            // This prevents items in shops from being burned
            if ( !cn_data.pchr->isshopitem )
            {
                if ( 0 != reaffirm_attached_particles( cn_data.ichr ) )
                {
                    retval = true;
                }
            }
        }
    }

    //Do they hit each other?
    if(prt_can_hit_chr && 0 == cn_data.pchr->damage_timer)
    {
        bool dodged = false;

        //Does the character have a dodge ability?
        if(cn_data.pchr->hasPerk(Ego::Perks::DODGE)) {
            float dodgeChance = cn_data.pchr->getAttribute(Ego::Attribute::AGILITY);

            //Masterful Dodge Perk gives flat +10% dodge chance
            if(cn_data.pchr->hasPerk(Ego::Perks::MASTERFUL_DODGE)) {
                dodgeChance += 10.0f;
            }

            //1% dodge chance per Agility
            if(Random::getPercent() <= dodgeChance) 
            {
                dodged = true;
            }
        }

        if(!dodged) {
            // do "damage" to the character
            if (!prt_deflected)
            {
                // we can't even get to this point if the character is completely invulnerable (invictus)
                // or can't be damaged this round
                cn_data.prt_damages_chr = do_chr_prt_collision_damage( &cn_data );
                if ( cn_data.prt_damages_chr )
                {
                    //Remember the collision so that this doesn't happen again
                    cn_data.pprt->addCollision(_currentModule->getObjectHandler()[cn_data.pchr->getCharacterID()]);
                    retval = true;
                }
            }

            //Cause knockback (Hold the Line perk makes Objects immune to knockback)
            if(!cn_data.pchr->hasPerk(Ego::Perks::HOLD_THE_LINE)) {
                do_chr_prt_collision_knockback(cn_data);
            }
        }

        //Attack was dodged!
        else {
            //Cannot collide again
            cn_data.pprt->addCollision(_currentModule->getObjectHandler()[cn_data.pchr->getCharacterID()]);

            //Play sound effect
            AudioSystem::get().playSound(cn_data.pchr->getPosition(), AudioSystem::get().getGlobalSound(GSND_DODGE));

            // Initialize for the billboard
            chr_make_text_billboard( cn_data.pchr->getCharacterID(), "Dodged!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f(1.0f, 0.6f, 0.0f, 1.0f), 3, Billboard::Flags::All);
        }


        // handle a couple of special cases (grabbing money)
        if (cn_data.prt_bumps_chr)
        {
            if ( do_chr_prt_collision_handle_bump(&cn_data) )
            {
                retval = true;
            }
        }
    }

    // terminate the particle if needed
    if ( cn_data.terminate_particle )
    {
        cn_data.pprt->requestTerminate();
        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void chr_prt_collision_data_t::init(chr_prt_collision_data_t& self)
{
    //---- invalidate the object parameters
    self.ichr = INVALID_CHR_REF;
    self.pchr = NULL;

    self.iprt = INVALID_PRT_REF;
    self.pprt = NULL;
    self.ppip = NULL;

    //---- collision parameters

    // true collisions
    self.int_min = 0;
    self.depth_min = 0.0f;

    // hit-box collisions
    self.int_max = 0;
    self.depth_max = 0.0f;

    // platform interactions
    //self.int_plat = false;
    //self.plat_lerp = 0.0f;

    // basic parameters
    self.is_impact    = false;
    self.is_pressure  = false;
    self.is_collision = false;
    self.dot = 0.0f;
    self.nrm = Vector3f(0, 0, 1);

    //---- collision modifications
    self.mana_paid = false;
    self.max_damage = self.actual_damage = 0;
    self.vdiff = Vector3f::zero();
    self.vdiff_para = Vector3f::zero();
    self.vdiff_perp = Vector3f::zero();
    self.block_factor = 0.0f;

    //---- collision reaction
    //self.vimpulse = Vector3f::zero();
    //self.pimpulse = Vector3f::zero();
    self.terminate_particle = false;
    self.prt_bumps_chr = false;
    self.prt_damages_chr = false;
}

//--------------------------------------------------------------------------------------------
static bool attach_prt_to_platform( Ego::Particle * pprt, Object * pplat )
{
    /// @author BB
    /// @details attach a particle to a platform

    // verify that we do not have two dud pointers
    if (!pprt || pprt->isTerminated() ) return false;
    if (!pplat || pplat->isTerminated()) return false;

    // check if they can be connected
    if ( !pplat->platform ) return false;

    // do the attachment
    pprt->onwhichplatform_ref    = pplat->getCharacterID();
    pprt->onwhichplatform_update = update_wld;
    pprt->targetplatform_ref     = INVALID_CHR_REF;

    // update the character's relationship to the ground
    pprt->setElevation( std::max( pprt->enviro.level, pplat->getPosZ() + pplat->chr_min_cv._maxs[OCT_Z] ) );

    return true;
}

//--------------------------------------------------------------------------------------------
bool detach_particle_from_platform( Ego::Particle * pprt )
{
    /// @author BB
    /// @details attach a particle to a platform


    // verify that we do not have two dud pointers
    if ( pprt == nullptr || pprt->isTerminated() ) return false;

    // grab all of the particle info
    prt_bundle_t bdl_prt(pprt);

    // undo the attachment
    pprt->onwhichplatform_ref    = INVALID_CHR_REF;
    pprt->onwhichplatform_update = 0;
    pprt->targetplatform_ref     = INVALID_CHR_REF;
    pprt->targetplatform_level   = -1e32;

    // get the correct particle environment
    bdl_prt.move_one_particle_get_environment();

    return true;
}
