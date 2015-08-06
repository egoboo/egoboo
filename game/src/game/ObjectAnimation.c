#include "ObjectAnimation.h"
#include "CharacterMatrix.h"
#include "ObjectPhysics.h"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct chr_anim_data_t
{
    chr_anim_data_t() :
        allowed(false),
        action(ACTION_DA),
        lip(0),
        speed(0.0f)
    {
        //ctor
    }

    bool allowed;
    int    action;
    int    lip;
    float  speed;
};

static int cmp_chr_anim_data( void const * vp_lhs, void const * vp_rhs );
static egolib_rv chr_invalidate_child_instances( Object * pchr );
static bool chr_handle_madfx( Object * pchr );
static float set_character_animation_rate( Object * pchr );

//--------------------------------------------------------------------------------------------
egolib_rv chr_set_action( Object * pchr, int action, bool action_ready, bool override_action )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = (egolib_rv)chr_instance_t::set_action(pchr->inst, action, action_ready, override_action);
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_start_anim( Object * pchr, int action, bool action_ready, bool override_action )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egolib_rv )chr_instance_t::start_anim(pchr->inst, action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_set_anim( Object * pchr, int action, int frame, bool action_ready, bool override_action )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egolib_rv )chr_instance_t::set_anim(pchr->inst, action, frame, action_ready, override_action);
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_increment_action( Object * pchr )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = (egolib_rv)chr_instance_t::increment_action(pchr->inst);
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_increment_frame( Object * pchr )
{
    egolib_rv retval;
    int mount_action;
    CHR_REF imount;
    bool needs_keep;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;
    imount = pchr->attachedto;

    // do we need to keep this animation?
    needs_keep = false;

    if ( !_currentModule->getObjectHandler().exists( imount ) )
    {
        imount = INVALID_CHR_REF;
        mount_action = ACTION_DA;
    }
    else
    {
        // determine what kind of action we are going to substitute for a riding character
        if ( _currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_LEFT] ) || _currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            // if the character is holding anything, make the animation
            // ACTION_MH == "sitting" so that it does not look so silly

            mount_action = pchr->getProfile()->getModel()->getAction(ACTION_MH);
            if ( ACTION_MH != mount_action )
            {
                // no real sitting animation. set the animation to keep
                needs_keep = true;
            }
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            mount_action = pchr->getProfile()->getModel()->getAction(ACTION_MI);
            if ( ACTION_MI != mount_action )
            {
                // no real riding animation. set the animation to keep
                needs_keep = true;
            }
        }
    }

    retval = ( egolib_rv )chr_instance_t::increment_frame(pchr->inst, imount, mount_action );
    if ( rv_success != retval ) return retval;

    /// @note BB@> this did not work as expected...
    // set keep if needed
    //if ( needs_keep )
    //{
    //    chr_instance_set_action_keep( &( pchr->inst ), true );
    //}

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_play_action( Object * pchr, int action, bool action_ready )
{
    if (!ACTIVE_PCHR(pchr)) {
        return rv_error;
    }

    egolib_rv retval = (egolib_rv)chr_instance_t::play_action(pchr->inst, action, action_ready);
    if (rv_success != retval) {
        return retval;
    }

    // if the instance is invalid, invalidate everything that depends on this object
    if (!pchr->inst.save.valid) {
        chr_invalidate_child_instances(pchr);
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_animation( Object * pchr )
{
    // Animate the character.
    // Right now there are 50/4 = 12.5 animation frames per second

    float flip_diff, flip_next;

    if ( NULL == pchr ) return;
    CHR_REF ichr  = GET_INDEX_PCHR( pchr );
    chr_instance_t& pinst = pchr->inst;

    flip_diff  = 0.25f * pinst.rate;

    flip_next = chr_instance_t::get_remaining_flip(pinst);

    while ( flip_next > 0.0f && flip_diff >= flip_next )
    {
        flip_diff -= flip_next;

        chr_instance_t::update_one_lip( pinst );

        // handle frame FX for the new frame
        if ( 3 == pinst.ilip )
        {
            chr_handle_madfx( pchr );
        }

        if ( 4 == pinst.ilip )
        {
            if ( rv_success != chr_increment_frame( pchr ) )
            {
                log_warning( "chr_increment_frame() did not succeed\n" );
            }
        }

        if ( pinst.ilip > 4 )
        {
            log_warning( "chr_increment_frame() - invalid ilip\n" );
            pinst.ilip = 0;
            break;
        }

        flip_next = chr_instance_t::get_remaining_flip( pinst );
    }

    if ( flip_diff > 0.0f )
    {
        int ilip_old = pinst.ilip;

        chr_instance_t::update_one_flip( pinst, flip_diff );

        if ( ilip_old != pinst.ilip )
        {
            // handle frame FX for the new frame
            if ( 3 == pinst.ilip )
            {
                chr_handle_madfx( pchr );
            }

            if ( 4 == pinst.ilip )
            {
                if ( rv_success != chr_increment_frame( pchr ) )
                {
                    log_warning( "chr_increment_frame() did not succeed\n" );
                }
            }

            if ( pinst.ilip > 4 )
            {
                log_warning( "chr_increment_frame() - invalid ilip\n" );
                pinst.ilip = 0;
            }
        }
    }

    set_character_animation_rate( pchr );
}


//--------------------------------------------------------------------------------------------
float set_character_animation_rate( Object * pchr )
{
    /// @author ZZ
    /// @details Get running, walking, sneaking, or dancing, from speed
    ///
    /// @author BB
    /// @details added automatic calculation of variable animation rates for movement animations

    float  speed;
    bool can_be_interrupted;
    bool is_walk_type;
    int    cnt, anim_count;
    int    action, lip;
    bool found;

    // set the character speed to zero
    speed = 0;

    if ( NULL == pchr ) return 1.0f;
    chr_instance_t& pinst = pchr->inst;
    CHR_REF ichr = GET_INDEX_PCHR(pchr);

    // if the action is set to keep then do nothing
    can_be_interrupted = !pinst.action_keep;
    if ( !can_be_interrupted ) return pinst.rate = 1.0f;

    // dont change the rate if it is an attack animation
    if ( pchr->isAttacking() )  return pinst.rate;

    // if the character is mounted or sitting, base the rate off of the mounr
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) && (( ACTION_MI == pinst.action_which ) || ( ACTION_MH == pinst.action_which ) ) )
    {
        // just copy the rate from the mount
        pinst.rate = _currentModule->getObjectHandler().get(pchr->attachedto)->inst.rate;
        return pinst.rate;
    }

    // if the animation is not a walking-type animation, ignore the variable animation rates
    // and the automatic determination of the walk animation
    // "dance" is walking with zero speed
    is_walk_type = ACTION_IS_TYPE( pinst.action_which, D ) || ACTION_IS_TYPE( pinst.action_which, W );
    if ( !is_walk_type ) return pinst.rate = 1.0f;

    // if the action cannot be changed on the at this time, there's nothing to do.
    // keep the same animation rate
    if ( !pinst.action_ready )
    {
        if ( 0.0f == pinst.rate ) pinst.rate = 1.0f;
        return pinst.rate;
    }

    // go back to a base animation rate, in case the next frame is not a
    // "variable speed frame"
    pinst.rate = 1.0f;

    // for non-flying objects, you have to be touching the ground
    if ( !pchr->enviro.grounded && !pchr->isFlying() ) return pinst.rate;

    // get the model
    const std::shared_ptr<Ego::ModelDescriptor> pmad = pchr->getProfile()->getModel();

    //---- set up the anim_info structure
    chr_anim_data_t anim_info[CHR_MOVEMENT_COUNT];
    anim_info[CHR_MOVEMENT_STOP ].speed = 0;
    anim_info[CHR_MOVEMENT_SNEAK].speed = pchr->anim_speed_sneak;
    anim_info[CHR_MOVEMENT_WALK ].speed = pchr->anim_speed_walk;
    anim_info[CHR_MOVEMENT_RUN  ].speed = pchr->anim_speed_run;

    if ( pchr->isFlying() )
    {
        // for flying characters, you have to flap like crazy to stand still and
        // do nothing to move quickly
        anim_info[CHR_MOVEMENT_STOP ].action = ACTION_WC;
        anim_info[CHR_MOVEMENT_SNEAK].action = ACTION_WB;
        anim_info[CHR_MOVEMENT_WALK ].action = ACTION_WA;
        anim_info[CHR_MOVEMENT_RUN  ].action = ACTION_DA;
    }
    else
    {
        anim_info[CHR_MOVEMENT_STOP ].action = ACTION_DA;
        anim_info[CHR_MOVEMENT_SNEAK].action = ACTION_WA;
        anim_info[CHR_MOVEMENT_WALK ].action = ACTION_WB;
        anim_info[CHR_MOVEMENT_RUN  ].action = ACTION_WC;
    }

    anim_info[CHR_MOVEMENT_STOP ].lip = 0;
    anim_info[CHR_MOVEMENT_SNEAK].lip = LIPWA;
    anim_info[CHR_MOVEMENT_WALK ].lip = LIPWB;
    anim_info[CHR_MOVEMENT_RUN  ].lip = LIPWC;

    // set up the arrays that are going to
    // determine whether the various movements are allowed
    for ( cnt = 0; cnt < CHR_MOVEMENT_COUNT; cnt++ )
    {
        anim_info[cnt].allowed = HAS_SOME_BITS( pchr->movement_bits, 1 << cnt );
    }

    if ( ACTION_WA != pmad->getAction(ACTION_WA) )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_SNEAK].allowed = false;

        /// @note ZF@> small fix here, if there is no sneak animation, try to default to normal walk with reduced animation speed
        if ( HAS_SOME_BITS( pchr->movement_bits, CHR_MOVEMENT_BITS_SNEAK ) )
        {
            anim_info[CHR_MOVEMENT_WALK].allowed = true;
            anim_info[CHR_MOVEMENT_WALK].speed *= 2;
        }
    }

    if ( ACTION_WB != pmad->getAction(ACTION_WB) )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_WALK].allowed = false;
    }

    if ( ACTION_WC != pmad->getAction(ACTION_WC) )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_RUN].allowed = false;
    }

    // sort the allowed movement(s) data
    qsort( anim_info, CHR_MOVEMENT_COUNT, sizeof( chr_anim_data_t ), cmp_chr_anim_data );

    // count the allowed movements
    for ( cnt = 0, anim_count = 0; cnt < CHR_MOVEMENT_COUNT; cnt++ )
    {
        if ( anim_info[cnt].allowed ) anim_count++;
    }

    // nothing to be done
    if ( 0 == anim_count )
    {
        return pinst.rate;
    }

    // estimate our speed
    if ( pchr->isFlying() )
    {
        // for flying objects, the speed is the actual speed
        speed = pchr->vel.length_abs();
    }
    else
    {
        // For non-flying objects, we use the intended speed.
        // new_v[kX], new_v[kY] is the speed before any latches are applied.
        speed = fvec2_t(pchr->enviro.new_v[kX], pchr->enviro.new_v[kY]).length_abs();
        if ( pchr->enviro.is_slipping )
        {
            // The character is slipping as on ice.
            // Make his little legs move based on his intended speed, for comic effect! :)
            speed *= 2.0f;
        }

    }

    if ( pchr->fat != 0.0f ) speed /= pchr->fat;

    // handle a special case
    if ( 1 == anim_count )
    {
        if ( 0.0f != anim_info[0].speed )
        {
            pinst.rate = speed / anim_info[0].speed;
        }

        return pinst.rate;
    }

    // search for the correct animation
    action = ACTION_DA;
    lip    = 0;
    found  = false;
    for ( cnt = 0; cnt < anim_count - 1; cnt++ )
    {
        float speed_mid = 0.5f * ( anim_info[cnt].speed + anim_info[cnt+1].speed );

        // make a special case for dance animation(s)
        if ( anim_info[cnt].speed <= FLT_EPSILON && speed <= 1e-3 )
        {
            found = true;
        }
        else
        {
            found = ( speed < speed_mid );
        }

        if ( found )
        {
            action = anim_info[cnt].action;
            lip    = anim_info[cnt].lip;
            if ( 0.0f != anim_info[cnt].speed )
            {
                pinst.rate = speed / anim_info[cnt].speed;
            }
            break;
        }
    }

    if ( !found )
    {
        action = anim_info[cnt].action;
        lip    = anim_info[cnt].lip;
        if ( 0.0f != anim_info[cnt].speed )
        {
            pinst.rate = speed / anim_info[cnt].speed;
        }
        found = true;
    }

    if ( ACTION_DA == action )
    {
        // Do standstill

        // handle boredom
        pchr->bore_timer--;
        if ( pchr->bore_timer < 0 )
        {
            int tmp_action, rand_val;

            SET_BIT( pchr->ai.alert, ALERTIF_BORED );
            pchr->bore_timer = BORETIME;

            // set the action to "bored", which is ACTION_DB, ACTION_DC, or ACTION_DD
            rand_val   = Random::next(std::numeric_limits<uint16_t>::max());
            tmp_action = pinst.imad->getAction(ACTION_DB + ( rand_val % 3 ));
            chr_start_anim( pchr, tmp_action, true, true );
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if ( !ACTION_IS_TYPE( pinst.action_which, D ) )
            {
                // get an appropriate version of the boredom action
                int tmp_action = pinst.imad->getAction(ACTION_DA);

                // start the animation
                chr_start_anim( pchr, tmp_action, true, true );
            }
        }
    }
    else
    {
        int tmp_action = pinst.imad->getAction(action);
        if ( ACTION_COUNT != tmp_action )
        {
            if ( pinst.action_which != tmp_action )
            {
                const MD2_Frame &nextFrame  = chr_instance_t::get_frame_nxt(pchr->inst);
                chr_set_anim( pchr, tmp_action, pmad->getFrameLipToWalkFrame(lip, nextFrame.framelip), true, true );
            }

            // "loop" the action
            chr_instance_t::set_action_next(pinst, tmp_action);
        }
    }

    //Limit final animation speed
    pinst.rate = Ego::Math::constrain(pinst.rate, 0.1f, 3.0f);

    return pinst.rate;
}


//--------------------------------------------------------------------------------------------
int cmp_chr_anim_data( void const * vp_lhs, void const * vp_rhs )
{
    /// @author BB
    /// @details Sort MOD REF values based on the rank of the module that they point to.
    ///               Trap all stupid values.

    chr_anim_data_t * plhs = ( chr_anim_data_t * )vp_lhs;
    chr_anim_data_t * prhs = ( chr_anim_data_t * )vp_rhs;

    int retval = 0;

    if ( NULL == plhs && NULL == prhs )
    {
        return 0;
    }
    else if ( NULL == plhs )
    {
        return 1;
    }
    else if ( NULL == prhs )
    {
        return -1;
    }

    retval = ( int )prhs->allowed - ( int )plhs->allowed;
    if ( 0 != retval ) return retval;

    retval = SGN( plhs->speed - prhs->speed );

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_invalidate_child_instances( Object * pchr )
{
    int cnt;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    // invalidate vlst_cache of everything in this character's holdingwhich array
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        CHR_REF iitem = pchr->holdingwhich[cnt];
        if ( !_currentModule->getObjectHandler().exists( iitem ) ) continue;

        // invalidate the matrix_cache
        _currentModule->getObjectHandler().get(iitem)->inst.matrix_cache.valid = false;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool chr_handle_madfx( Object * pchr )
{
    ///@details This handles special commands an animation frame might execute, for example
    ///         grabbing stuff or spawning attack particles.

    CHR_REF ichr;
    Uint32 framefx;

    if ( nullptr == pchr ) return false;

    framefx = chr_instance_t::get_framefx(pchr->inst);
    if ( 0 == framefx ) return true;

    ichr    = GET_INDEX_PCHR( pchr );

    // Check frame effects
    if ( HAS_SOME_BITS( framefx, MADFX_ACTLEFT ) )
    {
        character_swipe( ichr, SLOT_LEFT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_ACTRIGHT ) )
    {
        character_swipe( ichr, SLOT_RIGHT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, false );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, false );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, true );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, true );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPLEFT ) )
    {
        if(pchr->getLeftHandItem()) {
            pchr->getLeftHandItem()->detatchFromHolder(false, true);
        }
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPRIGHT ) )
    {
        if(pchr->getRightHandItem()) {
            pchr->getRightHandItem()->detatchFromHolder(false, true);
        }
    }

    if ( HAS_SOME_BITS( framefx, MADFX_POOF ) && !pchr->isPlayer() )
    {
        pchr->ai.poof_time = update_wld;
    }

    //Do footfall sound effect
    if (egoboo_config_t::get().sound_footfallEffects_enable.getValue() && HAS_SOME_BITS(framefx, MADFX_FOOTFALL))
    {
        AudioSystem::get().playSound(pchr->getPosition(), pchr->getProfile()->getFootFallSound());
    }

    return true;
}
