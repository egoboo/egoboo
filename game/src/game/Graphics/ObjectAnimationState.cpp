#include "ObjectAnimationState.hpp"
#include "game/Entities/_Include.hpp"
#include "game/ObjectAnimation.h"
#include "game/game.h" //only for character_swipe()

namespace Ego
{
namespace Graphics
{

ObjectAnimationState::ObjectAnimationState(Object &object) : 
    _object(object),
    _modelDescriptor(nullptr),
    _targetFrameIndex(0),
    _sourceFrameIndex(0),
    ilip(0),
    flip(0.0f),
    rate(1.0f) 
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
    ilip = 0;
    flip = 0.0f;
    rate = 1.0f;
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

void ObjectAnimationState::updateAnimation()
{
    float flip_diff  = 0.25f * this->rate;
    float flip_next = getRemainingFlip();

    while ( flip_next > 0.0f && flip_diff >= flip_next )
    {
        flip_diff -= flip_next;

        //Update one linear interpolated frame
        this->ilip += 1;
        this->flip = 0.25f * this->ilip;

        // handle frame FX for the new frame
        if ( 3 == this->ilip )
        {
            handleAnimationFX();
        }

        if ( 4 == this->ilip )
        {
            incrementFrame();
        }

        if ( this->ilip > 4 )
        {
            Log::get().warn( "chr_increment_frame() - invalid ilip\n" );
            this->ilip = 0;
            break;
        }

        flip_next = getRemainingFlip();
    }

    if ( flip_diff > 0.0f )
    {
        int ilip_old = this->ilip;

        // update the lips
        this->flip += flip_diff;
        this->ilip = ((int)std::floor(this->flip * 4)) % 4;

        if ( ilip_old != this->ilip )
        {
            // handle frame FX for the new frame
            if ( 3 == this->ilip )
            {
                handleAnimationFX();
            }

            if ( 4 == this->ilip )
            {
                incrementFrame();
            }

            if ( this->ilip > 4 )
            {
                Log::get().warn( "chr_increment_frame() - invalid ilip\n" );
                this->ilip = 0;
            }
        }
    }

    set_character_animation_rate(&_object);
}

float ObjectAnimationState::getRemainingFlip() const
{
    return (this->ilip + 1) * 0.25f - this->flip;
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
    this->ilip %= 4;
    this->flip = fmod(this->flip, 1.0f);

    // Change frames
    int frame_lst = getTargetFrameIndex();
    int frame_nxt = getTargetFrameIndex() + 1;

    // detect the end of the animation and handle special end conditions
    if (frame_nxt > getModelDescriptor()->getLastFrame(_object.inst.actionState.action_which))
    {
        if (_object.inst.actionState.action_keep)
        {
            // Freeze that animation at the last frame
            frame_nxt = frame_lst;

            // Break a kept action at any time
            _object.inst.actionState.action_ready = true;
        }
        else if (_object.inst.actionState.action_loop)
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
            frame_nxt = getModelDescriptor()->getFirstFrame(_object.inst.actionState.action_which);

            // Break a looped action at any time
            _object.inst.actionState.action_ready = true;
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
        chr_invalidate_child_instances(&_object);
    }
}

bool ObjectAnimationState::startAnimation(const ModelAction action, const bool action_ready, const bool override_action)
{
    gfx_rv retval = _object.inst.setAction(action, action_ready, override_action);
    if ( rv_success != retval ) return false;

    if(!setFrame(getModelDescriptor()->getFirstFrame(action))) {
        return false;
    }

    // if the instance is invalid, invalidate everything that depends on this object
    if (!_object.inst.isVertexCacheValid()) {
        chr_invalidate_child_instances(&_object);
    }

    return true;
}

bool ObjectAnimationState::setFrame(int frame)
{
    if (_object.inst.actionState.action_which < 0 || _object.inst.actionState.action_which > ACTION_COUNT) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, _object.inst.actionState.action_which, "invalid action range");
        return false;
    }

    // is the frame within the valid range for this action?
    if(!getModelDescriptor()->isFrameValid(_object.inst.actionState.action_which, frame)) {
        return false;
    }

    // jump to the next frame
    this->flip = 0.0f;
    this->ilip = 0;
    this->setSourceFrameIndex(getTargetFrameIndex());
    this->setTargetFrameIndex(frame);

    return true;
}

bool ObjectAnimationState::incrementAction()
{
    // get the correct action
    ModelAction action = getModelDescriptor()->getAction(_object.inst.actionState.action_next);

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    // @note ZF> Can't use ACTION_IS_TYPE(action, D) because of GCC compile warning
    bool action_ready = action < ACTION_DD || ACTION_IS_TYPE(action, W);

    return startAnimation(action, action_ready, true);
}

} //namespace Graphics
} //namespace Ego