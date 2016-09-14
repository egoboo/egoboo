#pragma once
#include "IdLib/IdLib.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "egolib/Graphics/MD2Model.hpp"

//Forward declaration
class Object;

namespace Ego
{
namespace Graphics
{

/// An animation state represents the interpolation state between to frames.
/// The interpolation is represented by an integer-valued interpolation state
/// \f$i \in [0,4]\f$ and a real-valued interpolatio state \f$r \in [0,1]\f$.
/// Those states are not independent i.e. if one state is changed then the other
/// state is changed as well. Their dependency is denoted by the formulas
/// \f$i = 4 r\f$ and \f$\frac{1}{4} i = r\f$ respectively.
class ObjectAnimationState : public Id::NonCopyable {
public:
    /// Construct this animation state.
    ObjectAnimationState(Object &object);

    /// Destruct this animation state.
    ~ObjectAnimationState();

    /// Get the model descriptor.
    /// @return the model descriptor
    const std::shared_ptr<Ego::ModelDescriptor>& getModelDescriptor() const;

    /// Set the model descriptor.
    /// @param modelDescriptor the model descriptor
    void setModelDescriptor(const std::shared_ptr<Ego::ModelDescriptor>& modelDescriptor);

    /// @brief Get the index of the source frame.
    /// @return the index of the source frame
    int getSourceFrameIndex() const;

    /// @brief Set the index of the source frame.
    /// @param sourceFrameIndex the index of the source frame
    void setSourceFrameIndex(int sourceFrameIndex);

    /// @brief Get the index of the target frame.
    /// @return the index of the target frame
    int getTargetFrameIndex() const;

    /// @brief Set the index of the target frame.
    /// @param targetFrameIndex the index of the target frame
    void setTargetFrameIndex(int targetFrameIndex);

    const MD2_Frame& getTargetFrame() const;

    const MD2_Frame& getSourceFrame() const;

    void reset();

    /**
    * @brief
    *   Animate the character.
    *   Right now there are 50/4 = 12.5 animation frames per second
    **/
    void updateAnimation();

    bool startAnimation(const ModelAction action, const bool action_ready, const bool override_action);

    bool setFrame(int frame);

    /**
    * @brief
    *   Set to true if the current animation can be interrupted by another animation
    **/
    void setActionReady(bool val) {
        _canBeInterrupted = val;
    }

    /**
    * @brief
    *   Set to true if the current animation should freeze at its final frame
    **/
    void setActionKeep(bool val) {
        _freezeAtLastFrame = val;
    }

    /**
    * @brief
    *   Set to true if the current animation action should be looped
    **/
    void setActionLooped(bool val) {
        _loopAnimation = val;
    }

    /**
    * @return
    *   true if the current animation can be interrupted by starting another animation
    **/
    bool canBeInterrupted() const;

    bool setAction(const ModelAction action, const bool action_ready, const bool override_action);

    bool setFrameFull(int frame_along, int ilip);

    ModelAction getCurrentAnimation() const;

    void setAnimationSpeed(const float rate);
    
    void removeInterpolation();

    /**
    * @brief
    *   Get the interpolated bounding box for the current animation frame. The current animation frame
    *   might have a different bounding box (like an arm reaching out for example)
    **/
    oct_bb_t getBoundingBox() const;

    float getFlip() const { return _animationProgress; }

private:
    void assertFrameIndex(int frameIndex) const;

    float getRemainingFlip() const;

    ///@details This handles special commands an animation frame might execute, for example
    ///         grabbing stuff or spawning attack particles.

    bool handleAnimationFX() const;

    /// @details all the code necessary to move on to the next frame of the animation
    void incrementFrame();

    /// @details This function starts the next action for a character
    bool incrementAction();

    /// @details Get running, walking, sneaking, or dancing, from speed
    ///          added automatic calculation of variable animation rates for movement animations
    void updateAnimationRate();

private:
    Object &_object;

    /// The model descriptor.
    std::shared_ptr<Ego::ModelDescriptor> _modelDescriptor;

    float _animationRate;           //< The animation rate (how fast does it play?)
    float _animationProgress; //=0.0f beginning of an animation frame, =1.0f reached next frame in animation
    uint8_t _animationProgressInteger; /// The integer-valued frame in betweening.

    /// The target frame index.
    uint16_t _targetFrameIndex;

    /// The source frame index.
    uint16_t _sourceFrameIndex;

    bool _canBeInterrupted;     //< Can this animation action be interrupted by another?
    bool _freezeAtLastFrame;    //< Freeze animation on the last frame?
    bool _loopAnimation;        //< true if the current animation should be replayed after the last frame
    
    ModelAction _currentAnimation;  //< The current animation which is playing
    ModelAction _nextAnimation;     //< The animation to play after current one is done
};

} //namespace Graphics
} //namespace Ego
