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

#include "file_formats/pip_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_camera;

//--------------------------------------------------------------------------------------------
// Particle graphic data
//--------------------------------------------------------------------------------------------
/// All the data necessary to diaplay a partile
struct s_prt_instance
{
    bool_t valid;

    // basic info
    Uint8    type;               ///< particle type
    Uint16   image;              ///< which image
    float    alpha;              ///< base alpha
    Uint8    light;              ///< base self lighting

    // position info
    fvec3_t   pos;
    float     size;
    float     scale;

    // billboard info
    prt_ori_t orientation;
    fvec3_t   up;
    fvec3_t   right;
    fvec3_t   nrm;

    // lighting info
    float    famb;               ///< cached ambient light
    float    fdir;               ///< cached directional light

    float    fintens;            ///< current brightness
    float    falpha;             ///< current alpha

    // graphical optimizations
    bool_t         indolist;        ///< Has it been added yet?

    // pre-compute some values for the reflected particle posisions
    fvec3_t ref_up;
    fvec3_t ref_right;
    fvec3_t ref_pos;
};
typedef struct s_prt_instance prt_instance_t;

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_solid( Uint16 iprt );
bool_t render_one_prt_trans( Uint16 iprt );
bool_t render_one_prt_ref( Uint16 iprt );

void   render_prt( struct s_camera * pcam );
void   render_prt_ref( struct s_camera * pcam );

void   render_all_prt_attachment();


void prt_instance_update_all( struct s_camera * pcam );
