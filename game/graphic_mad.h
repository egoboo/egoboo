#pragma once

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

#include "egoboo_typedef.h"
#include "egoboo_math.h"
#include "graphic.h"

#include "file_formats/cap_file.h"

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
enum e_matrix_cache_type
{
    MAT_UNKNOWN   = 0,
    MAT_CHARACTER = ( 1 << 0 ),
    MAT_WEAPON    = ( 1 << 1 )
};

typedef enum e_matrix_cache_type matrix_cache_type_t;

/// the data necessary to cache the last values required to create the character matrix
struct s_matrix_cache
{
    // is the cache data valid?
    bool_t valid;

    // is the matrix data valid?
    bool_t matrix_valid;

    // how was the matrix made?
    int type_bits;

    //---- MAT_CHARACTER data

    // the "Euler" rotation angles in 16-bit form
    fvec3_t   rotate;

    // the translate vector
    fvec3_t   pos;

    //---- MAT_WEAPON data

    Uint16  grip_chr;                   ///< !=MAX_CHR if character is a held weapon
    slot_t  grip_slot;                  ///< SLOT_LEFT or SLOT_RIGHT
    Uint16  grip_verts[GRIP_VERTS];     ///< Vertices which describe the weapon grip
    fvec3_t grip_scale;

    //---- data used for both

    // the body fixed scaling
    fvec3_t  self_scale;
};
typedef struct s_matrix_cache matrix_cache_t;

matrix_cache_t * matrix_cache_init( matrix_cache_t * mcache );

//--------------------------------------------------------------------------------------------
/// some pre-computed parameters for reflection
struct s_chr_reflection_cache
{
    fmat_4x4_t matrix;
    bool_t     matrix_valid;
    Uint8      alpha;
    Uint8      light;
    Uint8      sheen;
    Uint8      redshift;
    Uint8      grnshift;
    Uint8      blushift;

    Uint32     update_wld;
};
typedef struct s_chr_reflection_cache chr_reflection_cache_t;

//--------------------------------------------------------------------------------------------
/// the data to determine whether re-calculation of vlst is necessary
struct s_vlst_cache
{
    bool_t valid;             ///< do we know whether this cache is valid or not?

    float  flip;              ///< the in-betweening  the last time the animation was updated
    Uint16 frame_nxt;         ///< the initial frame  the last time the animation was updated
    Uint16 frame_lst;         ///< the final frame  the last time the animation was updated
    Uint32 frame_wld;         ///< the update_wld the last time the animation was updated

    int    vmin;              ///< the minimum clean vertex the last time the vertices were updated
    int    vmax;              ///< the maximum clean vertex the last time the vertices were updated
    Uint32 vert_wld;          ///< the update_wld the last time the vertices were updated
};
typedef struct s_vlst_cache vlst_cache_t;

vlst_cache_t * vlst_cache_init( vlst_cache_t * );

//--------------------------------------------------------------------------------------------
/// All the data that the renderer needs to draw the character
struct s_chr_instance
{
    // position info
    fmat_4x4_t     matrix;           ///< Character's matrix
    matrix_cache_t matrix_cache;     ///< Did we make one yet?

    Uint16         turn_z;

    // render mode info
    Uint8          alpha;           ///< 255 = Solid, 0 = Invisible
    Uint8          light;           ///< 1 = Light, 0 = Normal
    Uint8          sheen;           ///< 0-15, how shiny it is
    bool_t         enviro;          ///< Environment map?

    // color info
    Uint8          redshift;        ///< Color channel shifting
    Uint8          grnshift;
    Uint8          blushift;

    // texture info
    Uint16         texture;         ///< The texture id of the character's skin
    Uint16         uoffset;         ///< For moving textures
    Uint16         voffset;

    // model info
    Uint16         imad;            ///< Character's model

    // animation info
    Uint16         frame_nxt;       ///< Character's frame
    Uint16         frame_lst;       ///< Character's last frame
    Uint8          ilip;            ///< Character's frame in betweening
    float          flip;            ///< Character's frame in betweening
    float          rate;

    // action info
    Uint8          action_ready;                   ///< Ready to play a new one
    Uint8          action_which;                   ///< Character's action
    bool_t         action_keep;                    ///< Keep the action playing
    bool_t         action_loop;                    ///< Loop it too
    Uint8          action_next;                    ///< Character's action to play next

    // lighting info
    Sint32         color_amb;
    fvec4_t        col_amb;
    int            max_light, min_light;
    Uint32         lighting_update_wld;            ///< update some lighting info no more than once an update
    Uint32         lighting_frame_all;             ///< update some lighting info no more than once a frame

    // linear interpolated frame vertices
    size_t         vlst_size;
    GLvertex     * vlst;
    oct_bb_t       bbox;                           ///< the bounding box for this frame

    // graphical optimizations
    bool_t                 indolist;               ///< Has it been added yet?
    vlst_cache_t           save;                   ///< Do we need to re-calculate all or part of the vertex list
    chr_reflection_cache_t ref;                    ///< pre-computing some reflection parameters

    // OBSOLETE
    // lighting
    // Uint16         light_turn_z;    ///< Character's light rotation 0 to 0xFFFF
    // Uint8          lightlevel_amb;  ///< 0-255, terrain light
    // Uint8          lightlevel_dir;  ///< 0-255, terrain light
};

typedef struct s_chr_instance chr_instance_t;

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_enviro( Uint16 character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad_tex( Uint16 character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad( Uint16 character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad_ref( int tnc );


void      update_all_chr_instance();
egoboo_rv chr_update_instance( struct s_chr * pchr );
egoboo_rv chr_instance_update_bbox( chr_instance_t * pinst );
egoboo_rv chr_instance_needs_update( chr_instance_t * pinst, int vmin, int vmax, bool_t *verts_match, bool_t *frames_match );
egoboo_rv chr_instance_update_vertices( chr_instance_t * pinst, int vmin, int vmax, bool_t force );
egoboo_rv chr_instance_update_grip_verts( chr_instance_t * pinst, Uint16 vrt_lst[], size_t vrt_count );

egoboo_rv chr_instance_set_action( chr_instance_t * pinst, int action, bool_t action_ready, bool_t override );
egoboo_rv chr_instance_set_frame( chr_instance_t * pinst, int frame );
