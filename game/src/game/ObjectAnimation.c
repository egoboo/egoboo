#include "game/ObjectAnimation.h"
#include "game/CharacterMatrix.h"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/game.h"

//--------------------------------------------------------------------------------------------
float set_character_animation_rate( const Object * ptr )
{
    /// @author ZZ
    /// @details Get running, walking, sneaking, or dancing, from speed
    ///
    /// @author BB
    /// @details added automatic calculation of variable animation rates for movement animations

    bool is_walk_type;

    auto objRef = GET_INDEX_PCHR( ptr );
    const auto &pchr = _currentModule->getObjectHandler()[objRef];

    if ( NULL == pchr ) return 1.0f;
    ObjectGraphics& pinst = pchr->inst;

    // if the action is set to keep then do nothing
    if (pinst.actionState.action_keep) {
        pinst.animationState.rate = 1.0f;
        return 1.0f;
    }

    // dont change the rate if it is an attack animation
    if ( pchr->isAttacking() ) {  
        return pinst.animationState.rate;
    }

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
                ModelAction tmp_action = pinst.animationState.getModelDescriptor()->getAction(ACTION_DB + ( rand_val % 3 ));
                pchr->inst.animationState.startAnimation(tmp_action, true, true );
            }
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if ( !ACTION_IS_TYPE( pinst.actionState.action_which, D ) )
            {
                // get an appropriate version of the idle action
                ModelAction tmp_action = pinst.animationState.getModelDescriptor()->getAction(ACTION_DA);

                // start the animation
                pchr->inst.animationState.startAnimation(tmp_action, true, true );
            }
        }
    }
    else
    {
        ModelAction tmp_action = pinst.animationState.getModelDescriptor()->getAction(action);
        if ( ACTION_COUNT != tmp_action )
        {
            if ( pinst.actionState.action_which != tmp_action )
            {
                pinst.setAction(tmp_action, true, true);
                pinst.animationState.setFrame(pmad->getFrameLipToWalkFrame(lip, pinst.getNextFrame().framelip));
                pinst.animationState.startAnimation(tmp_action, true, true);
            }

            // "loop" the action
            pinst.setNextAction(tmp_action);
        }
    }

    //Limit final animation speed
    pinst.animationState.rate = Ego::Math::constrain(pinst.animationState.rate, 0.1f, 3.0f);

    return pinst.animationState.rate;
}


//--------------------------------------------------------------------------------------------
egolib_rv chr_invalidate_child_instances( const Object * pchr )
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

