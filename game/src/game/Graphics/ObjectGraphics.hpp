#pragma once

#include "idlib/idlib.hpp"
#include "game/CharacterMatrix.h"
#include "game/Graphics/Vertex.hpp"

#include "egolib/Graphics/ModelDescriptor.hpp"
#include "egolib/Graphics/MD2Model.hpp"

//Forward declarations
namespace Ego { namespace Graphics { class ObjectGraphics; } }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// is the coordinate system right handed or left handed?

#if defined(MAD_CULL_RIGHT)
// this worked with the old camera
#    define MAD_REF_CULL   Ego::WindingMode::AntiClockwise
#    define MAD_NRM_CULL   Ego::WindingMode::Clockwise
#else
// they had to be reversed with the new camera
#    define MAD_REF_CULL   Ego::WindingMode::Clockwise
#    define MAD_NRM_CULL   Ego::WindingMode::AntiClockwise
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Bits used to request a character tint
enum e_chr_render_bits
{
    CHR_UNKNOWN  = 0,
    CHR_SOLID    = ( 1 << 0 ),
    CHR_ALPHA    = ( 1 << 1 ),
    CHR_LIGHT    = ( 1 << 2 ),
    CHR_PHONG    = ( 1 << 3 ),
    CHR_REFLECT  = ( 1 << 4 )
};

//--------------------------------------------------------------------------------------------

struct colorshift_t {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    colorshift_t(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : red(red), green(green), blue(blue) {}
    colorshift_t(const colorshift_t& other)
        : red(other.red), green(other.green), blue(other.blue) {}
    colorshift_t& operator=(const colorshift_t& other) {
        red = other.red;
        green = other.green;
        blue = other.blue;
        return *this;
    }
};

//--------------------------------------------------------------------------------------------

/// the data to determine whether re-calculation of vlst is necessary
struct VertexListCache
{
    VertexListCache() :
        flip(0.0f),
        frame_nxt(0),
        frame_lst(0),
        frame_wld(0),
        vmin(-1),
        vmax(-1),
        vert_wld(0)
    {
        //ctor
    }

    void clear()
    {
        flip = 0.0f;
        frame_nxt = 0;
        frame_lst = 0;
        frame_wld = 0;
        vmin = -1;
        vmax = -1;
        vert_wld = 0;
    }

    float  flip;              ///< the in-betweening  the last time the animation was updated
    uint16_t frame_nxt;         ///< the initial frame  the last time the animation was updated
    uint16_t frame_lst;         ///< the final frame  the last time the animation was updated
    uint32_t frame_wld;         ///< the update_wld the last time the animation was updated

    int    vmin;              ///< the minimum clean vertex the last time the vertices were updated
    int    vmax;              ///< the maximum clean vertex the last time the vertices were updated
    uint32_t vert_wld;          ///< the update_wld the last time the vertices were updated
};

namespace Ego
{

namespace Graphics
{

/// All the data that the renderer needs to draw the character
class ObjectGraphics : private id::non_copyable
{
public:
    // position info
    matrix_cache_t matrix_cache;     ///< Did we make one yet?

    // render mode info
    uint8_t          alpha;                 ///< 255 = Solid, 0 = Invisible
    uint8_t          light;                 ///< 1 = Light, 0 = Normal
    uint8_t          sheen;                 ///< 0-15, how shiny it is

    // color channel shifting
    colorshift_t     colorshift;

    // texture info
    SFP8_T uoffset;                               ///< For moving textures (8.8 fixed point)
    SFP8_T voffset;                               ///< For moving textures (8.8 fixed point)

public:
	ObjectGraphics(Object& object);
    ~ObjectGraphics();

    /// @details determine the basic per-vertex lighting
	void updateLighting();

    bool isVertexCacheValid() const;

    bool updateGripVertices(const uint16_t vrt_lst[], const size_t vrt_count);

    bool playAction(const ModelAction action, const bool actionready);

	BIT_FIELD getFrameFX() const;

    gfx_rv updateVertices(int vmin, int vmax, bool force);
        
    void getTint(GLXvector4f tint, const bool reflection, const int type);

    const GLvertex& getVertex(const size_t index) const;

    size_t getVertexCount() const;

    /**
    * @brief 
    *   This function makes the character flash, feet one color, head another.
    *   This function sets a character's lighting depending on vertex height...
    *   Can make feet dark and head light...
    * @param valuelow 
    *     Brightness on the low vertices (0 to 255)
    * @param low
    *     The vertex index where the low vertices starts
    * @param valuehigh 
    *     Brightness on the top vertices (0 to 255)
    * @param high
    *     The vertex index where the high vertices starts
    **/
    void flashVariableHeight(const uint8_t valuelow, const int16_t low, const uint8_t valuehigh, const int16_t high);

    /// This function sets a object's lighting.
    void flash(uint8_t value);

    void setObjectProfile(const std::shared_ptr<ObjectProfile> &profile);

    /**
    * @brief
    *   Compute and return the reflection transparency based on altitude above the floor
    **/
    uint8_t getReflectionAlpha() const;

    /**
    * @brief
    *   Get the object 3D model matrix reflected into the floor
    **/
    const Matrix4f4f& getReflectionMatrix() const;

    /**
    * @brief
    *   Get the object 3D model matrix
    **/
    const Matrix4f4f& getMatrix() const;

    void setMatrix(const Matrix4f4f& matrix);

    int getMaxLight() const;

    int getAmbientColour() const;

    /// Get the model descriptor.
    /// @return the model descriptor
    const std::shared_ptr<ModelDescriptor>& getModelDescriptor() const;

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

private:

    /// Set the model descriptor.
    /// @param modelDescriptor the model descriptor
    void setModelDescriptor(const std::shared_ptr<ModelDescriptor>& modelDescriptor);

    /**
    * @brief
    *   Get the desired/next frame in the current animation action
    **/
    const MD2_Frame& getNextFrame() const;

    /**
    * @brief
    *   Get the current/previous frame in the current animation action
    **/
    const MD2_Frame& getLastFrame() const;

	gfx_rv updateVertexCache(int vmax, int vmin, bool force, bool vertices_match, bool frames_match);

    /**
    * @brief 
    *   determine whether some specific vertices of an instance need to be updated
    * @return
    *   gfx_error   means that the function was passed invalid values
    *   gfx_fail    means that the instance does not need to be updated
    *   gfx_success means that the instance should be updated
    **/
	gfx_rv needs_update(int vmin, int vmax, bool *verts_match, bool *frames_match);

    /**
    * @brief
    *   force chr_instance_update_vertices() recalculate the vertices the next time
    *   the function is called
    **/
	void clearCache();

	void interpolateVerticesRaw(const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip);

    /**
    * @brief
    *   try to set the model used by the character instance.
    **/
    bool setModel(const std::shared_ptr<ModelDescriptor> &imad);

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
    Object& _object;
    std::vector<GLvertex> _vertexList;
    Matrix4f4f _matrix;                     ///< Character's matrix
    Matrix4f4f _reflectionMatrix;           ///< Character's matrix reflecter (on the floor)

    // graphical optimizations
    VertexListCache _vertexCache;              ///< Do we need to re-calculate all or part of the vertex list

    // lighting info
    int32_t        _ambientColour;
    int            _maxLight;
    int            _lastLightingUpdateFrame;            ///< update some lighting info no more than once an update

    /// The model descriptor.
    std::shared_ptr<Ego::ModelDescriptor> _modelDescriptor;

    /// An animation state represents the interpolation state between to frames.
    /// The interpolation is represented by an integer-valued interpolation state
    /// \f$i \in [0,4]\f$ and a real-valued interpolatio state \f$r \in [0,1]\f$.
    /// Those states are not independent i.e. if one state is changed then the other
    /// state is changed as well. Their dependency is denoted by the formulas
    /// \f$i = 4 r\f$ and \f$\frac{1}{4} i = r\f$ respectively.
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
