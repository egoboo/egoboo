#include "ObjectAnimationState.hpp"
#include "game/Entities/_Include.hpp"
#include "game/ObjectAnimation.h"
#include "game/game.h" //only for character_swipe()

namespace Ego
{
namespace Graphics
{

static void chr_invalidate_child_instances(Object &object)
{
    if(object.getLeftHandItem()) {
        object.getLeftHandItem()->inst.matrix_cache.valid = false;
    }
    if(object.getRightHandItem()) {
        object.getRightHandItem()->inst.matrix_cache.valid = false;
    }
}

ObjectAnimationState::ObjectAnimationState(Object &object) : 
    _object(object),
    _modelDescriptor(nullptr),
    _animationRate(1.0f),
    _animationProgress(0.0f),
    _animationProgressInteger(0),
    _targetFrameIndex(0),
    _sourceFrameIndex(0),

    _canBeInterrupted(true),
    _freezeAtLastFrame(false),
    _loopAnimation(false),
    _currentAnimation(ACTION_DA),
    _nextAnimation(ACTION_DA)
{
    //ctor
}

ObjectAnimationState::~ObjectAnimationState()
{
    //dtor
}

void ObjectAnimationState::reset()
{
    _modelDescriptor = nullptr;
    _targetFrameIndex = 0;
    _sourceFrameIndex = 0;
    _animationProgressInteger = 0;
    _animationProgress = 0.0f;
    _animationRate = 1.0f;

    //Action specific variables
    _canBeInterrupted = true;
    _freezeAtLastFrame = false;
    _loopAnimation = false;
    _currentAnimation = ACTION_DA;
    _nextAnimation = ACTION_DA;
}

const std::shared_ptr<Ego::ModelDescriptor>& ObjectAnimationState::getModelDescriptor() const {
    return _modelDescriptor;
}

void ObjectAnimationState::setModelDescriptor(const std::shared_ptr<Ego::ModelDescriptor>& modelDescriptor) {
    _modelDescriptor = modelDescriptor;
}

int ObjectAnimationState::getSourceFrameIndex() const {
    return _sourceFrameIndex;
}
void ObjectAnimationState::setSourceFrameIndex(int sourceFrameIndex) {
    _sourceFrameIndex = sourceFrameIndex;
}
int ObjectAnimationState::getTargetFrameIndex() const {
    return _targetFrameIndex;
}

void ObjectAnimationState::setTargetFrameIndex(int targetFrameIndex) {
    _targetFrameIndex = targetFrameIndex;
}
const MD2_Frame& ObjectAnimationState::getTargetFrame() const {
    assertFrameIndex(getTargetFrameIndex());
    return getModelDescriptor()->getMD2()->getFrames()[getTargetFrameIndex()];
}

const MD2_Frame& ObjectAnimationState::getSourceFrame() const {
    assertFrameIndex(getSourceFrameIndex());
    return getModelDescriptor()->getMD2()->getFrames()[getSourceFrameIndex()];
}

void ObjectAnimationState::assertFrameIndex(int frameIndex) const {
    if (frameIndex > getModelDescriptor()->getMD2()->getFrames().size()) {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid frame " << frameIndex << "/" << getModelDescriptor()->getMD2()->getFrames().size() << Log::EndOfEntry;
        Log::get() << e;
        throw Id::RuntimeErrorException(__FILE__, __LINE__, e.getText());
    }
}

void ObjectAnimationState::setAnimationSpeed(const float rate)
{
    _animationRate = Ego::Math::constrain(rate, 0.1f, 3.0f);
}

void ObjectAnimationState::updateAnimation()
{
    float flip_diff  = 0.25f * _animationRate;
    float flip_next = getRemainingFlip();

    while ( flip_next > 0.0f && flip_diff >= flip_next )
    {
        flip_diff -= flip_next;

        //Update one linear interpolated frame
        _animationProgressInteger += 1;
        _animationProgress = 0.25f * _animationProgressInteger;

        // handle frame FX for the new frame
        if ( 3 == _animationProgressInteger )
        {
            handleAnimationFX();
        }

        if ( 4 == _animationProgressInteger )
        {
            incrementFrame();
        }

        if ( _animationProgressInteger > 4 )
        {
            Log::get().warn( "chr_increment_frame() - invalid ilip\n" );
            _animationProgressInteger = 0;
            break;
        }

        flip_next = getRemainingFlip();
    }

    if ( flip_diff > 0.0f )
    {
        int ilip_old = _animationProgressInteger;

        // update the lips
        _animationProgress += flip_diff;
        _animationProgressInteger = ((int)std::floor(_animationProgress * 4)) % 4;

        if ( ilip_old != _animationProgressInteger )
        {
            // handle frame FX for the new frame
            if ( 3 == _animationProgressInteger )
            {
                handleAnimationFX();
            }

            if ( 4 == _animationProgressInteger )
            {
                incrementFrame();
            }

            if ( _animationProgressInteger > 4 )
            {
                Log::get().warn( "chr_increment_frame() - invalid ilip\n" );
                _animationProgressInteger = 0;
            }
        }
    }

    updateAnimationRate();
}

float ObjectAnimationState::getRemainingFlip() const
{
    return (_animationProgressInteger + 1) * 0.25f - _animationProgress;
}

bool ObjectAnimationState::handleAnimationFX() const
{
    uint32_t framefx = _object.inst.getFrameFX();

    if ( 0 == framefx ) return true;

    // Check frame effects
    if ( HAS_SOME_BITS( framefx, MADFX_ACTLEFT ) )
    {
        character_swipe( _object.getObjRef(), SLOT_LEFT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_ACTRIGHT ) )
    {
        character_swipe( _object.getObjRef(), SLOT_RIGHT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABLEFT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_LEFT, false);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABRIGHT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_RIGHT, false);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARLEFT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_LEFT, true);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARRIGHT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_RIGHT, true);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPLEFT ) )
    {
        if(_object.getLeftHandItem()) {
            _object.getLeftHandItem()->detatchFromHolder(false, true);
        }
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPRIGHT ) )
    {
        if(_object.getRightHandItem()) {
            _object.getRightHandItem()->detatchFromHolder(false, true);
        }
    }

    if ( HAS_SOME_BITS( framefx, MADFX_POOF ) && !_object.isPlayer() )
    {
        _object.ai.poof_time = update_wld;
    }

    //Do footfall sound effect
    if (egoboo_config_t::get().sound_footfallEffects_enable.getValue() && HAS_SOME_BITS(framefx, MADFX_FOOTFALL))
    {
        AudioSystem::get().playSound(_object.getPosition(), _object.getProfile()->getFootFallSound());
    }

    return true;
}

void ObjectAnimationState::incrementFrame()
{
    // fix the ilip and flip
    _animationProgressInteger %= 4;
    _animationProgress = fmod(_animationProgress, 1.0f);

    // Change frames
    int frame_lst = getTargetFrameIndex();
    int frame_nxt = getTargetFrameIndex() + 1;

    // detect the end of the animation and handle special end conditions
    if (frame_nxt > getModelDescriptor()->getLastFrame(_currentAnimation))
    {
        if (_freezeAtLastFrame)
        {
            // Freeze that animation at the last frame
            frame_nxt = frame_lst;

            // Break a kept action at any time
            _canBeInterrupted = true;
        }
        else if (_loopAnimation)
        {
            // Convert the action into a riding action if the character is mounted
            if (_object.isBeingHeld())
            {
                ModelAction mount_action;

                // determine what kind of action we are going to substitute for a riding character
                if (_object.getLeftHandItem() || _object.getRightHandItem()) {
                    // if the character is holding anything, make the animation
                    // ACTION_MH == "sitting" so that it does not look so silly
                    mount_action = _object.getProfile()->getModel()->getAction(ACTION_MH);
                }
                else {
                    // if it is not holding anything, go for the riding animation
                    mount_action = _object.getProfile()->getModel()->getAction(ACTION_MI);
                }
                
                startAnimation(mount_action, true, true);
            }

            // set the frame to the beginning of the action
            frame_nxt = getModelDescriptor()->getFirstFrame(_currentAnimation);

            // Break a looped action at any time
            _canBeInterrupted = true;
        }
        else
        {
            // Go on to the next action. don't let just anything interrupt it?
            incrementAction();

            // incrementAction() actually sets this value properly. just grab the new value.
            frame_nxt = getTargetFrameIndex();
        }
    }

    setSourceFrameIndex(frame_lst);
    setTargetFrameIndex(frame_nxt);

    // if the instance is invalid, invalidate everything that depends on this object
    if (!_object.inst.isVertexCacheValid()) {
        chr_invalidate_child_instances(_object);
    }
}

bool ObjectAnimationState::startAnimation(const ModelAction action, const bool action_ready, const bool override_action)
{
    if (!setAction(action, action_ready, override_action)) {
        return false;
    }

    if(!setFrame(getModelDescriptor()->getFirstFrame(action))) {
        return false;
    }

    // if the instance is invalid, invalidate everything that depends on this object
    if (!_object.inst.isVertexCacheValid()) {
        chr_invalidate_child_instances(_object);
    }

    return true;
}

bool ObjectAnimationState::setFrame(int frame)
{
    // is the frame within the valid range for this action?
    if(!getModelDescriptor()->isFrameValid(_currentAnimation, frame)) {
        return false;
    }

    // jump to the next frame
    _animationProgress = 0.0f;
    _animationProgressInteger = 0;
    setSourceFrameIndex(getTargetFrameIndex());
    setTargetFrameIndex(frame);

    return true;
}

bool ObjectAnimationState::incrementAction()
{
    // get the correct action
    ModelAction action = getModelDescriptor()->getAction(_nextAnimation);

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    // @note ZF> Can't use ACTION_IS_TYPE(action, D) because of GCC compile warning
    bool action_ready = action < ACTION_DD || ACTION_IS_TYPE(action, W);

    return startAnimation(action, action_ready, true);
}

void ObjectAnimationState::updateAnimationRate()
{
    ObjectGraphics& pinst = _object.inst;

    // dont change the rate if it is an attack animation
    if ( _object.isAttacking() ) {  
        return;
    }

    // if the action is set to keep then do nothing
    if (_freezeAtLastFrame) {
        return;
    }

    // if the action cannot be changed on the at this time, there's nothing to do.
    // keep the same animation rate
    if ( !canBeInterrupted() )
    {
        if (0.0f == _animationRate) { 
            _animationRate = 1.0f;
        }
        return;
    }

    // go back to a base animation rate, in case the next frame is not a
    // "variable speed frame"
    _animationRate = 1.0f;

    // if the character is mounted or sitting, base the rate off of the mounr
    if ( _object.isBeingHeld() && (( ACTION_MI == _currentAnimation ) || ( ACTION_MH == _currentAnimation ) ) )
    {
        if(_object.getHolder()->isScenery()) {
            //This is a special case to make animation while in the Pot (which is actually a "mount") look better
            _animationRate = 0.0f;
        }
        else {
            // just copy the rate from the mount
            _animationRate = _object.getHolder()->inst.animationState._animationRate;
        }

        return;
    }

    // if the animation is not a walking-type animation, ignore the variable animation rates
    // and the automatic determination of the walk animation
    // "dance" is walking with zero speed
    bool is_walk_type = (_currentAnimation < ACTION_DD) || ACTION_IS_TYPE( _currentAnimation, W );
    if ( !is_walk_type ) {
        return;
    }

    // for non-flying objects, you have to be touching the ground
    if (!_object.getObjectPhysics().isTouchingGround() && !_object.isFlying()) {
        return;
    }

    // set the character speed to zero
    float speed = 0.0f;

    // estimate our speed
    if ( _object.isFlying() )
    {
        // for flying objects, the speed is the actual speed
        speed = _object.vel.length();
    }
    else
    {
        // For non-flying objects, we use the intended speed.
        speed = std::max(std::sqrt(_object.vel.x()*_object.vel.x() + _object.vel.y()*_object.vel.y()), _object.getObjectPhysics().getDesiredVelocity().length());
        if (_object.getObjectPhysics().floorIsSlippy())
        {
            // The character is slipping as on ice.
            // Make his little legs move based on his intended speed, for comic effect! :)
            _animationRate = 2.0f;
            speed *= 2.0f;
        }

    }

    //Make bigger Objects have slower animations
    if ( _object.fat > 0.0f ) {
        speed /= _object.fat;  
    }

    //Find out which animation to use depending on movement speed
    ModelAction action = ACTION_DA;
    int lip = 0;
    if (speed <= 1.0f) {
        action = ACTION_DA;     //Stand still
    }
    else {
        if(_object.isStealthed() && getModelDescriptor()->isActionValid(ACTION_WA)) {
            action = ACTION_WA; //Sneak animation
            lip = LIPWA;
        }
        else {
            if(speed <= 4.0f && getModelDescriptor()->isActionValid(ACTION_WB)) {
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
    if ( _object.isFlying() )
    {
        switch(action)
        {
            case ACTION_DA: action = ACTION_WC; break;
            case ACTION_WA: action = ACTION_WB; break;
            case ACTION_WB: action = ACTION_WA; break;
            case ACTION_WC: action = ACTION_DA; break;
            default: /*don't modify action*/    break;
        }
    }

    if ( ACTION_DA == action )
    {
        // Do standstill

        // handle boredom
        _object.bore_timer--;
        if ( _object.bore_timer < 0 )
        {
            _object.resetBoredTimer();

            //Don't yell "im bored!" while stealthed!
            if(!_object.isStealthed())
            {
                SET_BIT( _object.ai.alert, ALERTIF_BORED );

                // set the action to "bored", which is ACTION_DB, ACTION_DC, or ACTION_DD
                int rand_val   = Random::next(std::numeric_limits<uint16_t>::max());
                ModelAction tmp_action = getModelDescriptor()->getAction(ACTION_DB + ( rand_val % 3 ));
                _object.inst.animationState.startAnimation(tmp_action, true, true );
            }
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if (_currentAnimation > ACTION_DD)
            {
                // get an appropriate version of the idle action
                ModelAction tmp_action = getModelDescriptor()->getAction(ACTION_DA);

                // start the animation
                startAnimation(tmp_action, true, true );
            }
        }
    }
    else
    {
        ModelAction tmp_action = getModelDescriptor()->getAction(action);
        if ( ACTION_COUNT != tmp_action )
        {
            if ( _currentAnimation != tmp_action )
            {
                setAction(tmp_action, true, true);
                setFrame(getModelDescriptor()->getFrameLipToWalkFrame(lip, pinst.getNextFrame().framelip));
                startAnimation(tmp_action, true, true);
            }

            // "loop" the action
            _nextAnimation = tmp_action;
        }
    }

    //Limit final animation speed
    setAnimationSpeed(_animationRate);
}

bool ObjectAnimationState::canBeInterrupted() const
{
    return _canBeInterrupted;    
}

bool ObjectAnimationState::setAction(const ModelAction action, const bool action_ready, const bool override_action)
{
    // is the chosen action valid?
    if (!getModelDescriptor()->isActionValid(action)) {
        return false;
    }

    // are we going to check action_ready?
    if (!override_action && !_canBeInterrupted) {
        return false;
    }

    // set up the action
    _currentAnimation = action;
    _nextAnimation = ACTION_DA;
    _canBeInterrupted = action_ready;

    return true;
}

bool ObjectAnimationState::setFrameFull(int frame_along, int ilip)
{
    // we have to have a valid action range
    if (_currentAnimation > ACTION_COUNT) {
        return false;
    }

    // try to heal a bad action
    _currentAnimation = getModelDescriptor()->getAction(_currentAnimation);

    // reject the action if it is cannot be made valid
    if (_currentAnimation == ACTION_COUNT) {
        return false;
    }

    // get some frame info
    int frame_stt   = getModelDescriptor()->getFirstFrame(_currentAnimation);
    int frame_end   = getModelDescriptor()->getLastFrame(_currentAnimation);
    int frame_count = 1 + ( frame_end - frame_stt );

    // try to heal an out of range value
    frame_along %= frame_count;

    // get the next frames
    int new_nxt = frame_stt + frame_along;
    new_nxt = std::min(new_nxt, frame_end);

    setTargetFrameIndex(new_nxt);
    _animationProgressInteger = ilip;
    _animationProgress = _animationProgressInteger * 0.25f;

    // set the validity of the cache
    return true;
}

ModelAction ObjectAnimationState::getCurrentAnimation() const
{
    return _currentAnimation;
}

void ObjectAnimationState::removeInterpolation()
{
    if (getSourceFrameIndex() != getTargetFrameIndex() ) {
        setSourceFrameIndex(getTargetFrameIndex());
        _animationProgressInteger = 0;
        _animationProgress = 0.0f;
    }
}

oct_bb_t ObjectAnimationState::getBoundingBox() const
{
    //Beginning of a frame animation
    if (getTargetFrameIndex() == getSourceFrameIndex() || _animationProgress == 0.0f) {
        return _object.inst.getLastFrame().bb;
    } 

    //Finished frame animation
    if (_animationProgress == 1.0f) {
        return _object.inst.getNextFrame().bb;
    } 

    //We are middle between two animation frames
    return oct_bb_t::interpolate(_object.inst.getLastFrame().bb, _object.inst.getNextFrame().bb, _animationProgress);
}

} //namespace Graphics
} //namespace Ego