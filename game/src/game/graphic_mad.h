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

#include "game/egoboo_typedef.h"
#include "game/graphic.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

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
//--------------------------------------------------------------------------------------------

/// Bits that tell you which variables to look at
enum matrix_cache_type_t
{
    MAT_UNKNOWN   = 0,
    MAT_CHARACTER = ( 1 << 0 ),
    MAT_WEAPON    = ( 1 << 1 )
};

/// the data necessary to cache the last values required to create the character matrix
struct matrix_cache_t
{
    matrix_cache_t() :
        valid(false),
        matrix_valid(false),
        type_bits(MAT_UNKNOWN),
        rotate(0.0f, 0.0f, 0.0f),
        pos(),
        grip_chr(),
        grip_slot(SLOT_LEFT),
        grip_verts(),
        grip_scale(),
        self_scale()
    {
        grip_verts.fill(0xFFFF);
    }

    // is the cache data valid?
    bool valid;

    // is the matrix data valid?
    bool matrix_valid;

    // how was the matrix made?
    int type_bits;

    //---- MAT_CHARACTER data

    // the "Euler" rotation angles in 16-bit form
    Vector3f   rotate;

    // the translate vector
    Vector3f   pos;

    //---- MAT_WEAPON data

    ObjectRef grip_chr;                   ///< != INVALID_CHR_REF if character is a held weapon
    slot_t  grip_slot;                  ///< SLOT_LEFT or SLOT_RIGHT
    std::array<uint16_t, GRIP_VERTS> grip_verts;     ///< Vertices which describe the weapon grip
    Vector3f grip_scale;

    //---- data used for both

    // the body fixed scaling
    Vector3f  self_scale;
};

//--------------------------------------------------------------------------------------------

/// some pre-computed parameters for reflection
struct chr_reflection_cache_t
{
    chr_reflection_cache_t() :
        matrix(Matrix4f4f::identity()),
        matrix_valid(false),
        alpha(127),
        light(0xFF),
        sheen(0),
        redshift(0),
        grnshift(0),
        blushift(0),
        update_wld(0)
    {
        //ctor
    };

    Matrix4f4f matrix;
    bool       matrix_valid;
    uint8_t    alpha;
    uint8_t    light;
    uint8_t    sheen;
    uint8_t    redshift;
    uint8_t    grnshift;
    uint8_t    blushift;

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

/// All the data that the renderer needs to draw the character
struct chr_instance_t
{
    int update_frame;                ///< the last frame that the instance was calculated in

    // position info
    Matrix4f4f     matrix;           ///< Character's matrix
    matrix_cache_t matrix_cache;     ///< Did we make one yet?

    FACING_T       facing_z;

    // render mode info
    uint8_t          alpha;                 ///< 255 = Solid, 0 = Invisible
    uint8_t          light;                 ///< 1 = Light, 0 = Normal
    uint8_t          sheen;                 ///< 0-15, how shiny it is
    bool         enviro;                ///< Environment map?
    bool         dont_cull_backfaces;   ///< Do we cull backfaces for this character or not?
    bool         skin_has_transparency; ///< The object skin has partial transparency

    // color info
    uint8_t          redshift;        ///< Color channel shifting
    uint8_t          grnshift;
    uint8_t          blushift;

    // texture info
    oglx_texture_t* texture;        ///< The texture of the character's skin
    SFP8_T         uoffset;         ///< For moving textures (8.8 fixed point)
    SFP8_T         voffset;         ///< For moving textures (8.8 fixed point)

    // model info
    std::shared_ptr<Ego::ModelDescriptor> imad;            ///< Character's model

    // animation info
    uint16_t         frame_nxt;       ///< Character's frame
    uint16_t         frame_lst;       ///< Character's last frame
    uint8_t          ilip;            ///< Character's frame in betweening
    float          flip;            ///< Character's frame in betweening
    float          rate;

    // action info
    bool         action_ready;                   ///< Ready to play a new one
    int            action_which;                   ///< Character's action
    bool         action_keep;                    ///< Keep the action playing
    bool         action_loop;                    ///< Loop it too
    int            action_next;                    ///< Character's action to play next

    // lighting info
    int32_t         color_amb;
    Vector4f       col_amb;
    int            max_light, min_light;
    int            lighting_update_wld;            ///< update some lighting info no more than once an update
    int            lighting_frame_all;             ///< update some lighting info no more than once a frame

    // linear interpolated frame vertices
    size_t         vrt_count;
    GLvertex     * vrt_lst;
    oct_bb_t       bbox;                           ///< the bounding box for this frame

    // graphical optimizations
    bool                 indolist;               ///< Has it been added yet?
    vlst_cache_t           save;                   ///< Do we need to re-calculate all or part of the vertex list
    chr_reflection_cache_t ref;                    ///< pre-computing some reflection parameters

public:
	chr_instance_t();

	static chr_instance_t *dtor(chr_instance_t& self);

	/// This function sets a object's lighting.
	static void flash(chr_instance_t& self, uint8_t value);

	static gfx_rv increment_action(chr_instance_t& self);
	static gfx_rv play_action(chr_instance_t& self, int action, bool actionready);
	static void set_action_keep(chr_instance_t& self, bool val);
	static void set_action_ready(chr_instance_t& self, bool val);
	static void set_action_loop(chr_instance_t& self, bool val);
	static gfx_rv set_action_next(chr_instance_t& self, int val);
	static gfx_rv set_action(chr_instance_t& self, int action, bool action_ready, bool override_action);

	static gfx_rv start_anim(chr_instance_t& self, int action, bool action_ready, bool override_action);
	static gfx_rv set_anim(chr_instance_t& self, int action, int frame, bool action_ready, bool override_action);

	static gfx_rv set_texture(chr_instance_t& self, const Ego::DeferredOpenGLTexture& itex);
	static gfx_rv set_mad(chr_instance_t& self, const std::shared_ptr<Ego::ModelDescriptor> &imad);

	static void update_ref(chr_instance_t& self, float grid_level, bool need_matrix);
	static gfx_rv update_bbox(chr_instance_t& self);
	static gfx_rv update_vertices(chr_instance_t& self, int vmin, int vmax, bool force);
	static gfx_rv update_grip_verts(chr_instance_t& self, Uint16 vrt_lst[], size_t vrt_count);
	static void update_one_lip(chr_instance_t& self);
	static gfx_rv update_one_flip(chr_instance_t& self, float dflip);
	static void update_lighting_base(chr_instance_t& self, Object *pchr, bool force);

	static gfx_rv spawn(chr_instance_t& self, const PRO_REF profile, const int skin);

	static gfx_rv increment_frame(chr_instance_t& self, const CHR_REF imount, const int mount_action);
	static void remove_interpolation(chr_instance_t& self);
	static gfx_rv set_frame_full(chr_instance_t& self, int frame_along, int ilip, const std::shared_ptr<Ego::ModelDescriptor> &mad_override);

	static const MD2_Frame& get_frame_nxt(const chr_instance_t& self);
	static const MD2_Frame& get_frame_lst(chr_instance_t& self);
	static BIT_FIELD get_framefx(const chr_instance_t& self);

	static float get_remaining_flip(chr_instance_t& self);
	static void get_tint(chr_instance_t& self, GLfloat *tint, const BIT_FIELD bits);
	static bool apply_reflection_matrix(chr_instance_t& self, float floor_level);


private:
	static gfx_rv alloc(chr_instance_t& self, size_t vlst_size);
	static void dealloc(chr_instance_t& self);
	
	static gfx_rv update_vlst_cache(chr_instance_t& self, int vmax, int vmin, bool force, bool vertices_match, bool frames_match);
	static gfx_rv needs_update(chr_instance_t& self, int vmin, int vmax, bool *verts_match, bool *frames_match);
	static gfx_rv set_frame(chr_instance_t& self, int frame);
	static void clear_cache(chr_instance_t& self);
	static void interpolate_vertices_raw(GLvertex dst_ary[], const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip);
};




//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct MadRenderer {
	static gfx_rv render(Camera& cam, ObjectRef ichr, GLXvector4f tint, const BIT_FIELD bits);
	/// @brief Draw object reflected in the floor.
	static gfx_rv render_ref(Camera& cam, ObjectRef ichr);
	/// @brief Dispatch rendering of transparent objects to the correct function.
	/// @remark Does not handle reflections in the floor.
	static gfx_rv render_trans(Camera& cam, ObjectRef ichr);
	static gfx_rv render_solid(Camera& cam, ObjectRef ichr);
private:
	/// Draw model with environment mapping.
	static gfx_rv render_enviro(Camera& cam, ObjectRef ichr, GLXvector4f tint, const BIT_FIELD bits);
	/// Draw model with texturing.
	static gfx_rv render_tex(Camera& cam, ObjectRef ichr, GLXvector4f tint, const BIT_FIELD bits);
};
