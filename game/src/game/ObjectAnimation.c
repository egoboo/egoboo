#include "game/ObjectAnimation.h"
#include "game/CharacterMatrix.h"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/game.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static egolib_rv chr_invalidate_child_instances( Object * pchr );
static bool chr_handle_madfx( Object * pchr );
static float set_character_animation_rate( Object * pchr );

//--------------------------------------------------------------------------------------------
egolib_rv chr_set_action( Object * pchr, int action, bool action_ready, bool override_action )
{
    egolib_rv retval;

    if (!pchr || pchr->isTerminated()) return rv_error;

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

    if (!pchr || pchr->isTerminated()) return rv_error;

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

    if (!pchr || pchr->isTerminated()) return rv_error;

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

    if (!pchr || pchr->isTerminated() ) return rv_error;

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
    
    if (!pchr || pchr->isTerminated() ) return rv_error;
    ObjectRef imount = pchr->attachedto;

    if ( !_currentModule->getObjectHandler().exists( imount ) )
    {
        imount = ObjectRef::Invalid;
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
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            mount_action = pchr->getProfile()->getModel()->getAction(ACTION_MI);
        }
    }

    retval = ( egolib_rv )chr_instance_t::increment_frame(pchr->inst, imount, mount_action );
    if ( rv_success != retval ) return retval;

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
    if (!pchr || pchr->isTerminated()) {
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
    chr_instance_t& pinst = pchr->inst;

    flip_diff  = 0.25f * pinst.animationState.rate;

    flip_next = chr_instance_t::get_remaining_flip(pinst);

    while ( flip_next > 0.0f && flip_diff >= flip_next )
    {
        flip_diff -= flip_next;

        chr_instance_t::update_one_lip( pinst );

        // handle frame FX for the new frame
        if ( 3 == pinst.animationState.ilip )
        {
            chr_handle_madfx( pchr );
        }

        if ( 4 == pinst.animationState.ilip )
        {
            if ( rv_success != chr_increment_frame( pchr ) )
            {
				Log::get().warn( "chr_increment_frame() did not succeed\n" );
            }
        }

        if ( pinst.animationState.ilip > 4 )
        {
			Log::get().warn( "chr_increment_frame() - invalid ilip\n" );
            pinst.animationState.ilip = 0;
            break;
        }

        flip_next = chr_instance_t::get_remaining_flip( pinst );
    }

    if ( flip_diff > 0.0f )
    {
        int ilip_old = pinst.animationState.ilip;

        chr_instance_t::update_one_flip( pinst, flip_diff );

        if ( ilip_old != pinst.animationState.ilip )
        {
            // handle frame FX for the new frame
            if ( 3 == pinst.animationState.ilip )
            {
                chr_handle_madfx( pchr );
            }

            if ( 4 == pinst.animationState.ilip )
            {
                if ( rv_success != chr_increment_frame( pchr ) )
                {
					Log::get().warn( "chr_increment_frame() did not succeed\n" );
                }
            }

            if ( pinst.animationState.ilip > 4 )
            {
				Log::get().warn( "chr_increment_frame() - invalid ilip\n" );
                pinst.animationState.ilip = 0;
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

    bool is_walk_type;

    if ( NULL == pchr ) return 1.0f;
    chr_instance_t& pinst = pchr->inst;

    // if the action is set to keep then do nothing
    if ( pinst.actionState.action_keep ) return pinst.animationState.rate = 1.0f;

    // dont change the rate if it is an attack animation
    if ( pchr->isAttacking() )  return pinst.animationState.rate;

    // if the character is mounted or sitting, base the rate off of the mounr
    if ( pchr->isBeingHeld() && (( ACTION_MI == pinst.actionState.action_which ) || ( ACTION_MH == pinst.actionState.action_which ) ) )
    {
        if(pchr->getHolder()->isScenery()) {
            //This is a special case to make animation while in the Pot (which is actually a "mount") look better
            pinst.animationState.rate = 0.0f;
        }
        else {
            // just copy the rate from the mount
            pinst.animationState.rate = pchr->getHolder()->inst.animationState.rate;
        }

        return pinst.animationState.rate;
    }

    // if the animation is not a walking-type animation, ignore the variable animation rates
    // and the automatic determination of the walk animation
    // "dance" is walking with zero speed
    is_walk_type = ACTION_IS_TYPE( pinst.actionState.action_which, D ) || ACTION_IS_TYPE( pinst.actionState.action_which, W );
    if ( !is_walk_type ) return pinst.animationState.rate = 1.0f;

    // if the action cannot be changed on the at this time, there's nothing to do.
    // keep the same animation rate
    if ( !pinst.actionState.action_ready )
    {
        if ( 0.0f == pinst.animationState.rate ) pinst.animationState.rate = 1.0f;
        return pinst.animationState.rate;
    }

    // go back to a base animation rate, in case the next frame is not a
    // "variable speed frame"
    pinst.animationState.rate = 1.0f;

    // for non-flying objects, you have to be touching the ground
    if (!pchr->getObjectPhysics().isTouchingGround() && !pchr->isFlying()) return pinst.animationState.rate;

    // get the model
    const std::shared_ptr<Ego::ModelDescriptor> pmad = pchr->getProfile()->getModel();

    // set the character speed to zero
    float speed = 0.0f;

    // estimate our speed
    if ( pchr->isFlying() )
    {
        // for flying objects, the speed is the actual speed
        speed = pchr->vel.length();
    }
    else
    {
        // For non-flying objects, we use the intended speed.
        speed = std::max(std::sqrt(pchr->vel.x()*pchr->vel.x() + pchr->vel.y()*pchr->vel.y()), pchr->getObjectPhysics().getDesiredVelocity().length());
        if (pchr->getObjectPhysics().floorIsSlippy())
        {
            // The character is slipping as on ice.
            // Make his little legs move based on his intended speed, for comic effect! :)
            pinst.animationState.rate = 2.0f;
            speed *= 2.0f;
        }

    }

    //Make bigger Objects have slower animations
    if ( pchr->fat > 0.0f ) speed /= pchr->fat;

    //Find out which animation to use depending on movement speed
    int action = ACTION_DA;
    int lip = 0;
    if (speed <= 1.0f) {
        action = ACTION_DA;     //Stand still
    }
    else {
        if(pchr->isStealthed() && pmad->isActionValid(ACTION_WA)) {
            action = ACTION_WA; //Sneak animation
            lip = LIPWA;
        }
        else {
            if(speed <= 4.0f && pmad->isActionValid(ACTION_WB)) {
                action = ACTION_WB; //Walk
                lip = LIPWB;
            }
            else {
                action = ACTION_WC; //Run
                lip = LIPWC;
            }

        }
    }

    // for flying characters, you have to flap like crazy to stand still and
    // do nothing to move quickly
    if ( pchr->isFlying() )
    {
        switch(action)
        {
            case ACTION_DA: action = ACTION_WC; break;
            case ACTION_WA: action = ACTION_WB; break;
            case ACTION_WB: action = ACTION_WA; break;
            case ACTION_WC: action = ACTION_DA; break;
        }
    }

    if ( ACTION_DA == action )
    {
        // Do standstill

        // handle boredom
        pchr->bore_timer--;
        if ( pchr->bore_timer < 0 )
        {
            pchr->resetBoredTimer();

            //Don't yell "im bored!" while stealthed!
            if(!pchr->isStealthed())
            {
                SET_BIT( pchr->ai.alert, ALERTIF_BORED );

                // set the action to "bored", which is ACTION_DB, ACTION_DC, or ACTION_DD
                int rand_val   = Random::next(std::numeric_limits<uint16_t>::max());
                int tmp_action = pinst.animationState.getModelDescriptor()->getAction(ACTION_DB + ( rand_val % 3 ));
                chr_start_anim( pchr, tmp_action, true, true );
            }
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if ( !ACTION_IS_TYPE( pinst.actionState.action_which, D ) )
            {
                // get an appropriate version of the idle action
                int tmp_action = pinst.animationState.getModelDescriptor()->getAction(ACTION_DA);

                // start the animation
                chr_start_anim( pchr, tmp_action, true, true );
            }
        }
    }
    else
    {
        int tmp_action = pinst.animationState.getModelDescriptor()->getAction(action);
        if ( ACTION_COUNT != tmp_action )
        {
            if ( pinst.actionState.action_which != tmp_action )
            {
                const MD2_Frame &nextFrame  = chr_instance_t::get_frame_nxt(pchr->inst);
                chr_set_anim( pchr, tmp_action, pmad->getFrameLipToWalkFrame(lip, nextFrame.framelip), true, true );
                chr_start_anim(pchr, tmp_action, true, true);
            }

            // "loop" the action
            chr_instance_t::set_action_next(pinst, tmp_action);
        }
    }

    //Limit final animation speed
    pinst.animationState.rate = Ego::Math::constrain(pinst.animationState.rate, 0.1f, 3.0f);

    return pinst.animationState.rate;
}


//--------------------------------------------------------------------------------------------
egolib_rv chr_invalidate_child_instances( Object * pchr )
{
    if (!pchr || pchr->isTerminated()) return rv_error;

    // invalidate vlst_cache of everything in this character's holdingwhich array
    for (size_t cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        ObjectRef iitem = pchr->holdingwhich[cnt];
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

    Uint32 framefx;

    if ( nullptr == pchr ) return false;

    framefx = chr_instance_t::get_framefx(pchr->inst);
    if ( 0 == framefx ) return true;

    auto objRef = GET_INDEX_PCHR( pchr );

    // Check frame effects
    if ( HAS_SOME_BITS( framefx, MADFX_ACTLEFT ) )
    {
        character_swipe( objRef, SLOT_LEFT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_ACTRIGHT ) )
    {
        character_swipe( objRef, SLOT_RIGHT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABLEFT ) )
    {
        pchr->getObjectPhysics().grabStuff(GRIP_LEFT, false);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABRIGHT ) )
    {
        pchr->getObjectPhysics().grabStuff(GRIP_RIGHT, false);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARLEFT ) )
    {
        pchr->getObjectPhysics().grabStuff(GRIP_LEFT, true);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARRIGHT ) )
    {
        pchr->getObjectPhysics().grabStuff(GRIP_RIGHT, true);
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
