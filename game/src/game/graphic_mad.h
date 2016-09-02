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

/// @file game/graphic_mad.h

#pragma once

#include "game/egoboo.h"
#include "game/graphic.h"
#include "CharacterMatrix.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Forward declarations
struct chr_instance_t;
class Camera;

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

/// some pre-computed parameters for reflection
struct chr_reflection_cache_t
{
    chr_reflection_cache_t() :
        matrix(Matrix4f4f::identity()),
        matrix_valid(false),
        alpha(127),
        light(0xFF),
        sheen(0),
        colorshift(),
        update_wld(0)
    {
        //ctor
    };

    Matrix4f4f matrix;
    bool       matrix_valid;
    uint8_t    alpha;
    uint8_t    light;
    uint8_t    sheen;
    colorshift_t colorshift;
    uint32_t   update_wld;
};

//--------------------------------------------------------------------------------------------

/// the data to determine whether re-calculation of vlst is necessary
struct vlst_cache_t
{
    vlst_cache_t() :
        valid(false),
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

    bool valid;             ///< do we know whether this cache is valid or not?

    float  flip;              ///< the in-betweening  the last time the animation was updated
    uint16_t frame_nxt;         ///< the initial frame  the last time the animation was updated
    uint16_t frame_lst;         ///< the final frame  the last time the animation was updated
    uint32_t frame_wld;         ///< the update_wld the last time the animation was updated

    int    vmin;              ///< the minimum clean vertex the last time the vertices were updated
    int    vmax;              ///< the maximum clean vertex the last time the vertices were updated
    uint32_t vert_wld;          ///< the update_wld the last time the vertices were updated
	static gfx_rv test(vlst_cache_t& self, chr_instance_t *instance);
};

//--------------------------------------------------------------------------------------------

/// An animation state represents the interpolation state between to frames.
/// The interpolation is represented by an integer-valued interpolation state
/// \f$i \in [0,4]\f$ and a real-valued interpolatio state \f$r \in [0,1]\f$.
/// Those states are not independent i.e. if one state is changed then the other
/// state is changed as well. Their dependency is denoted by the formulas
/// \f$i = 4 r\f$ and \f$\frac{1}{4} i = r\f$ respectively.
struct AnimationState {
private:
    /// The model descriptor.
    std::shared_ptr<Ego::ModelDescriptor> modelDescriptor;
    /// The target frame index.
    uint16_t targetFrameIndex;
    /// The source frame index.
    uint16_t sourceFrameIndex;

public:
    /// The integer-valued frame in betweening.
    uint8_t ilip;
    /// The real-valued frame in betweening.
    float flip; //flip=0.0f beginning of an animation frame, flip=1.0f reached next frame in animation
    /// The animation rate.
    float rate;
    /// Construct this animation state.
    AnimationState()
        : modelDescriptor(nullptr),
        targetFrameIndex(0),
        sourceFrameIndex(0),
        ilip(0),
        flip(0.0f),
        rate(1.0f) {}
    /// Destruct this animation state.
    ~AnimationState() {}
    /// Get the model descriptor.
    /// @return the model descriptor
    const std::shared_ptr<Ego::ModelDescriptor> getModelDescriptor() const {
        return modelDescriptor;
    }
    /// Set the model descriptor.
    /// @param modelDescriptor the model descriptor
    void setModelDescriptor(const std::shared_ptr<Ego::ModelDescriptor>& modelDescriptor) {
        this->modelDescriptor = modelDescriptor;
    }
    /// @brief Get the index of the source frame.
    /// @return the index of the source frame
    int getSourceFrameIndex() const {
        return sourceFrameIndex;
    }
    /// @brief Set the index of the source frame.
    /// @param sourceFrameIndex the index of the source frame
    void setSourceFrameIndex(int sourceFrameIndex) {
        this->sourceFrameIndex = sourceFrameIndex;
    }
    /// @brief Get the index of the target frame.
    /// @return the index of the target frame
    int getTargetFrameIndex() const {
        return targetFrameIndex;
    }
    /// @brief Set the index of the target frame.
    /// @param targetFrameIndex the index of the target frame
    void setTargetFrameIndex(int targetFrameIndex) {
        this->targetFrameIndex = targetFrameIndex;
    }
    const MD2_Frame& getTargetFrame() const {
        assertFrameIndex(getTargetFrameIndex());
        return getModelDescriptor()->getMD2()->getFrames()[getTargetFrameIndex()];
    }

    const MD2_Frame& getSourceFrame() const {
        assertFrameIndex(getSourceFrameIndex());
        return getModelDescriptor()->getMD2()->getFrames()[getSourceFrameIndex()];
    }
private:
    void assertFrameIndex(int frameIndex) const {
        if (frameIndex > getModelDescriptor()->getMD2()->getFrames().size()) {
            Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
            e << "invalid frame " << frameIndex << "/" << getModelDescriptor()->getMD2()->getFrames().size() << Log::EndOfEntry;
            Log::get() << e;
            throw Id::RuntimeErrorException(__FILE__, __LINE__, e.getText());
        }
    }
};

/// An action state.
struct ActionState {
    // action info
    /// Ready to play a new action.
    bool action_ready;
    /// The action playing.
    int action_which;
    /// Keep the action playing.
    bool action_keep;
    /// Loop the action.
    bool action_loop;
    /// The action to play next.
    int action_next;
    /// Construct this action state.
    ActionState()
        : action_ready(true),         // argh! this must be set at the beginning, script's spawn animations do not work!
        action_which(ACTION_DA),
        action_keep(false),
        action_loop(false),
        action_next(ACTION_DA) {}
    /// Destruct this action state.
    ~ActionState() {}
};

/// All the data that the renderer needs to draw the character
struct chr_instance_t
{
    // position info
    Matrix4f4f     matrix;           ///< Character's matrix
    matrix_cache_t matrix_cache;     ///< Did we make one yet?

    FACING_T       facing_z;

    // render mode info
    uint8_t          alpha;                 ///< 255 = Solid, 0 = Invisible
    uint8_t          light;                 ///< 1 = Light, 0 = Normal
    uint8_t          sheen;                 ///< 0-15, how shiny it is

    // color channel shifting
    colorshift_t     colorshift;

    // texture info
    SFP8_T uoffset;                               ///< For moving textures (8.8 fixed point)
    SFP8_T voffset;                               ///< For moving textures (8.8 fixed point)

    /// The animation state.
    AnimationState animationState;

    /// The action state.
    ActionState actionState;

    // lighting info
    int32_t         color_amb;
    Vector4f       col_amb;
    int            max_light, min_light;
    int            lighting_update_wld;            ///< update some lighting info no more than once an update
    int            lighting_frame_all;             ///< update some lighting info no more than once a frame

    // graphical optimizations
    vlst_cache_t           save;                   ///< Do we need to re-calculate all or part of the vertex list
    chr_reflection_cache_t ref;                    ///< pre-computing some reflection parameters

public:
	chr_instance_t();
    ~chr_instance_t();

    /**
    * @brief
    *   Set to true if the current animation should freeze at its final frame
    **/
	void setActionKeep(bool val);

    /**
    * @brief
    *   Set to true if the current animation can be interrupted by another animation
    **/
    void setActionReady(bool val);

    /**
    * @brief
    *   Set to true if the current animation action should be looped
    **/
	void setActionLooped(bool val);


	void setNextAction(const ModelAction val);

	gfx_rv setAction(const ModelAction action, const bool action_ready, const bool override_action);

	gfx_rv startAnimation(const ModelAction action, const bool action_ready, const bool override_action);

	static void update_ref(chr_instance_t& self, const Vector3f &position, bool need_matrix);
	static void update_lighting_base(chr_instance_t& self, Object *pchr, bool force);

    static gfx_rv increment_frame(chr_instance_t& self, const ObjectRef imount, const ModelAction mount_action);

    //Only used by CharacterAnimation.c
    static gfx_rv update_one_flip(chr_instance_t& self, float dflip);
    static gfx_rv update_grip_verts(chr_instance_t& self, Uint16 vrt_lst[], size_t vrt_count);
    static void update_one_lip(chr_instance_t& self);
    static gfx_rv play_action(chr_instance_t& self, int action, bool actionready);

	BIT_FIELD getFrameFX() const;

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

    /**
    * @brief
    *   Get the interpolated bounding box for the current animation frame. The current animation frame
    *   might have a different bounding box (like an arm reaching out for example)
    **/
    oct_bb_t getBoundingBox() const;

    gfx_rv updateVertices(int vmin, int vmax, bool force);
    
    void removeInterpolation();

    gfx_rv setFrameFull(int frame_along, int ilip);

	float getRemainingFlip() const;
    
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

    gfx_rv setFrame(int frame);

private:	
    /// @details This function starts the next action for a character
    gfx_rv incrementAction();

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
    bool setModel(const std::shared_ptr<Ego::ModelDescriptor> &imad);

private:
    std::vector<GLvertex> _vertexList;
};

//--------------------------------------------------------------------------------------------

struct MadRenderer {
	static gfx_rv render(Camera& cam, const std::shared_ptr<Object>& object, GLXvector4f tint, const BIT_FIELD bits);
    
	/// @brief Draw object reflected in the floor.
	static gfx_rv render_ref(Camera& cam, const std::shared_ptr<Object>& object);

	/// @brief Dispatch rendering of transparent objects to the correct function.
	/// @remark Does not handle reflections in the floor.
	static gfx_rv render_trans(Camera& cam, const std::shared_ptr<Object>& object);
	static gfx_rv render_solid(Camera& cam, const std::shared_ptr<Object>& object);

private:
	/// Draw model with environment mapping.
	static gfx_rv render_enviro(Camera& cam, const std::shared_ptr<Object>& object, GLXvector4f tint, const BIT_FIELD bits);
	/// Draw model with texturing.
	static gfx_rv render_tex(Camera& cam, const std::shared_ptr<Object>& object, GLXvector4f tint, const BIT_FIELD bits);

#if _DEBUG
    static void draw_chr_verts(const std::shared_ptr<Object>&pchr, int vrt_offset, int verts);
    static void _draw_one_grip_raw(chr_instance_t * pinst, int slot);
    static void draw_one_grip(chr_instance_t * pinst, int slot);
    //static void draw_chr_grips( Object * pchr );
    static void draw_chr_attached_grip(const std::shared_ptr<Object>& pchr);
    static void draw_chr_bbox(const std::shared_ptr<Object>& pchr);
#endif

};
