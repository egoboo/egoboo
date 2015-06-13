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

// Forward declaration.
class Camera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct GLvertex;
struct chr_instance_t;
struct vlst_cache_t;
struct matrix_cache_t;
struct chr_reflection_cache_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// is the coordinate system right handed or left handed?

#if defined(MAD_CULL_RIGHT)
// this worked with the old camera
#    define MAD_REF_CULL   GL_CCW
#    define MAD_NRM_CULL   GL_CW
#else
// they had to be reversed with the new camera
#    define MAD_REF_CULL   GL_CW
#    define MAD_NRM_CULL   GL_CCW
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

#define GRIP_VERTS             4

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
        type_bits(0),
        rotate(),
        pos(),
        grip_chr(INVALID_CHR_REF),
        grip_slot(SLOT_LEFT),
        grip_verts(),
        grip_scale(),
        self_scale()
    {
        //ctor
    }

	static matrix_cache_t *init(matrix_cache_t& self);

    // is the cache data valid?
    bool valid;

    // is the matrix data valid?
    bool matrix_valid;

    // how was the matrix made?
    int type_bits;

    //---- MAT_CHARACTER data

    // the "Euler" rotation angles in 16-bit form
    fvec3_t   rotate;

    // the translate vector
    fvec3_t   pos;

    //---- MAT_WEAPON data

    CHR_REF grip_chr;                   ///< != INVALID_CHR_REF if character is a held weapon
    slot_t  grip_slot;                  ///< SLOT_LEFT or SLOT_RIGHT
    std::array<uint16_t, GRIP_VERTS> grip_verts;     ///< Vertices which describe the weapon grip
    fvec3_t grip_scale;

    //---- data used for both

    // the body fixed scaling
    fvec3_t  self_scale;
};

//--------------------------------------------------------------------------------------------

/// some pre-computed parameters for reflection
struct chr_reflection_cache_t
{
    fmat_4x4_t matrix;
    bool     matrix_valid;
    Uint8      alpha;
    Uint8      light;
    Uint8      sheen;
    Uint8      redshift;
    Uint8      grnshift;
    Uint8      blushift;

    Uint32     update_wld;

	static chr_reflection_cache_t *init(chr_reflection_cache_t& self);
};

//--------------------------------------------------------------------------------------------

/// the data to determine whether re-calculation of vlst is necessary
struct vlst_cache_t
{
    bool valid;             ///< do we know whether this cache is valid or not?

    float  flip;              ///< the in-betweening  the last time the animation was updated
    Uint16 frame_nxt;         ///< the initial frame  the last time the animation was updated
    Uint16 frame_lst;         ///< the final frame  the last time the animation was updated
    Uint32 frame_wld;         ///< the update_wld the last time the animation was updated

    int    vmin;              ///< the minimum clean vertex the last time the vertices were updated
    int    vmax;              ///< the maximum clean vertex the last time the vertices were updated
    Uint32 vert_wld;          ///< the update_wld the last time the vertices were updated
	static vlst_cache_t *init(vlst_cache_t& self);
	static gfx_rv test(vlst_cache_t& self, chr_instance_t *instance);
};

//--------------------------------------------------------------------------------------------

/// All the data that the renderer needs to draw the character
struct chr_instance_t
{
    int update_frame;                ///< the last frame that the instance was calculated in

    // position info
    fmat_4x4_t     matrix;           ///< Character's matrix
    matrix_cache_t matrix_cache;     ///< Did we make one yet?

    FACING_T       facing_z;

    // render mode info
    Uint8          alpha;                 ///< 255 = Solid, 0 = Invisible
    Uint8          light;                 ///< 1 = Light, 0 = Normal
    Uint8          sheen;                 ///< 0-15, how shiny it is
    bool         enviro;                ///< Environment map?
    bool         dont_cull_backfaces;   ///< Do we cull backfaces for this character or not?
    bool         skin_has_transparency; ///< The object skin has partial transparency

    // color info
    Uint8          redshift;        ///< Color channel shifting
    Uint8          grnshift;
    Uint8          blushift;

    // texture info
    TX_REF         texture;         ///< The texture id of the character's skin
    SFP8_T         uoffset;         ///< For moving textures (8.8 fixed point)
    SFP8_T         voffset;         ///< For moving textures (8.8 fixed point)

    // model info
    MAD_REF        imad;            ///< Character's model

    // animation info
    Uint16         frame_nxt;       ///< Character's frame
    Uint16         frame_lst;       ///< Character's last frame
    Uint8          ilip;            ///< Character's frame in betweening
    float          flip;            ///< Character's frame in betweening
    float          rate;

    // action info
    bool         action_ready;                   ///< Ready to play a new one
    int            action_which;                   ///< Character's action
    bool         action_keep;                    ///< Keep the action playing
    bool         action_loop;                    ///< Loop it too
    int            action_next;                    ///< Character's action to play next

    // lighting info
    Sint32         color_amb;
    fvec4_t        col_amb;
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

    // OBSOLETE
    // lighting
    // FACING_T       light_turn_z;    ///< Character's light rotation 0 to 0xFFFF
    // Uint8          lightlevel_amb;  ///< 0-255, terrain light
    // Uint8          lightlevel_dir;  ///< 0-255, terrain light

	static chr_instance_t *ctor(chr_instance_t& self);
	static chr_instance_t *dtor(chr_instance_t& self);

	static gfx_rv increment_action(chr_instance_t& self);
	static gfx_rv play_action(chr_instance_t& self, int action, bool actionready);
	static void set_action_keep(chr_instance_t& self, bool val);
	static void set_action_ready(chr_instance_t& self, bool val);
	static void set_action_loop(chr_instance_t& self, bool val);
	static gfx_rv set_action_next(chr_instance_t& self, int val);
	static gfx_rv set_action(chr_instance_t& self, int action, bool action_ready, bool override_action);

	static gfx_rv start_anim(chr_instance_t& self, int action, bool action_ready, bool override_action);
	static gfx_rv set_anim(chr_instance_t& self, int action, int frame, bool action_ready, bool override_action);

	static gfx_rv set_texture(chr_instance_t& self, const TX_REF itex);
	static gfx_rv set_mad(chr_instance_t& self, const MAD_REF imad);

	static void update_ref(chr_instance_t& self, float grid_level, bool need_matrix);
	static gfx_rv update_bbox(chr_instance_t& self);
	static gfx_rv update_vertices(chr_instance_t& self, int vmin, int vmax, bool force);
	static gfx_rv update_grip_verts(chr_instance_t& self, Uint16 vrt_lst[], size_t vrt_count);
	static void update_one_lip(chr_instance_t& self);
	static gfx_rv update_one_flip(chr_instance_t& self, float dflip);
	static void update_lighting_base(chr_instance_t& self, Object *pchr, bool force);

	static gfx_rv spawn(chr_instance_t& self, const PRO_REF profile, const int skin);

	static gfx_rv increment_frame(chr_instance_t& self, mad_t *pmad, const CHR_REF imount, const int mount_action);
	static void remove_interpolation(chr_instance_t& self);
	static gfx_rv set_frame_full(chr_instance_t& self, int frame_along, int ilip, const MAD_REF mad_override);

	static const MD2_Frame& get_frame_nxt(chr_instance_t& self);
	static const MD2_Frame& get_frame_lst(chr_instance_t& self);
	static BIT_FIELD get_framefx(chr_instance_t& self);

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

void chr_instance_flash(chr_instance_t& self, Uint8 value);










//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_rv render_one_mad( Camera& cam, const CHR_REF ichr, GLXvector4f tint, const BIT_FIELD bits );
gfx_rv render_one_mad_ref( Camera& cam, const CHR_REF ichr );
gfx_rv render_one_mad_trans( Camera& cam, const CHR_REF ichr );
gfx_rv render_one_mad_solid( Camera& cam, const CHR_REF ichr );
