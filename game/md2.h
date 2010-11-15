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
///
/// @file md2.h
/// @brief Md2 Model display routines
/// @details Adapted from "Tactics - MD2_Model.h" by Jonathan Fischer
///   A class for loading/using Quake 2 and Egoboo md2 models.
///   Creating/destroying objects of this class is done in the same fashion as
///   Textures, so see Texture.h for details.
/// @note You will routinely include "md2.h" only in headers (*.h) files where you need to declare an
///       struct defined in this file. In *.inl files or *.c/*.cpp files you will routinely include "md2.inl", instead.

#include "id_md2.h"

#include "egoboo_typedef.h"
#include "physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EGO_NORMAL_COUNT  (MD2_MAX_NORMALS + 1)
#define EGO_AMBIENT_INDEX  MD2_MAX_NORMALS

//--------------------------------------------------------------------------------------------
// MD2_Vertex_t
//--------------------------------------------------------------------------------------------

struct s_ego_md2_vertex
{
    fvec3_t pos;
    fvec3_t nrm;
    int     normal;  ///< index to id-normal array

};
typedef struct s_ego_md2_vertex MD2_Vertex_t;

//--------------------------------------------------------------------------------------------
// MD2_TexCoord_t
//--------------------------------------------------------------------------------------------

struct s_ego_md2_texcoord
{
    fvec2_t tex;
};
typedef struct s_ego_md2_texcoord MD2_TexCoord_t;

//--------------------------------------------------------------------------------------------
// MD2_Frame_t
//--------------------------------------------------------------------------------------------

struct s_ego_md2_frame
{
    char          name[16];

    size_t        vertex_count;
    MD2_Vertex_t *vertex_lst;

    oct_bb_t      bb;             ///< axis-aligned octagonal bounding box limits
    int           framelip;       ///< the position in the current animation
    BIT_FIELD     framefx;        ///< the special effects associated with this frame
};
typedef struct s_ego_md2_frame MD2_Frame_t;

//--------------------------------------------------------------------------------------------
// MD2_Triangle_t
//--------------------------------------------------------------------------------------------

typedef id_md2_triangle_t MD2_Triangle_t;

//--------------------------------------------------------------------------------------------
// MD2_SkinName_t
//--------------------------------------------------------------------------------------------

typedef id_md2_skin_t MD2_SkinName_t;

//--------------------------------------------------------------------------------------------
// MD2_GLCommand_t
//--------------------------------------------------------------------------------------------

struct s_ego_md2_glcommand
{
    struct s_ego_md2_glcommand * next;

    GLenum              gl_mode;
    signed int          command_count;
    id_glcmd_packed_t * data;
};
typedef struct s_ego_md2_glcommand MD2_GLCommand_t;

void MD2_GLCommand_ctor( MD2_GLCommand_t * m );
void MD2_GLCommand_dtor( MD2_GLCommand_t * m );

MD2_GLCommand_t * MD2_GLCommand_create( void );
MD2_GLCommand_t * MD2_GLCommand_new_vector( int n );
void              MD2_GLCommand_destroy( MD2_GLCommand_t ** m );
void              MD2_GLCommand_delete_vector( MD2_GLCommand_t * v, int n );

//--------------------------------------------------------------------------------------------
// MD2_Model_t
//--------------------------------------------------------------------------------------------

struct s_ego_md2_model
{
    int m_numVertices;
    int m_numTexCoords;
    int m_numTriangles;
    int m_numSkins;
    int m_numFrames;
    int m_numCommands;

    MD2_SkinName_t  *m_skins;
    MD2_TexCoord_t  *m_texCoords;
    MD2_Triangle_t  *m_triangles;
    MD2_Frame_t     *m_frames;
    MD2_GLCommand_t *m_commands;

};
typedef struct s_ego_md2_model MD2_Model_t;

// CTORS
MD2_Model_t * MD2_Model_ctor( MD2_Model_t * m );
MD2_Model_t * MD2_Model_dtor( MD2_Model_t * m );
MD2_Model_t * MD2_Model_create( void );
void          MD2_Model_destroy( MD2_Model_t ** m );
MD2_Model_t * MD2_Model_new_vector( int n );
void          MD2_Model_delete_vector( MD2_Model_t * v, int n );

//--------------------------------------------------------------------------------------------
// EXTERNAL VARIABLES
//--------------------------------------------------------------------------------------------

extern float kMd2Normals[EGO_NORMAL_COUNT][3];

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

MD2_Model_t * md2_load( const char * szFilename, MD2_Model_t* m );
void          md2_free( MD2_Model_t * m );
void          md2_scale_model( MD2_Model_t * pmd2, float scale_x, float scale_y, float scale_z );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egoboo_md2_h
