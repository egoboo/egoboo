#pragma once

// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

/* Egoboo - id_md2.h
 * Md2 model file loading structures & constants.
 */

#include "egoboo_typedef.h"
#include "ogl_include.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum e_id_md2_constant
{
    MD2_MAGIC_NUMBER   = 0x32504449,
    MD2_VERSION        = 8,
    MD2_MAX_NORMALS    = 162,
    MD2_MAX_TRIANGLES  = 0x1000,
    MD2_MAX_VERTICES   = 0x0800,
    MD2_MAX_TEXCOORDS  = 0x0800,
    MD2_MAX_FRAMES     = 0x0200,
    MD2_MAX_SKINS      = 0x0020,
    MD2_MAX_FRAMESIZE  = ( MD2_MAX_VERTICES * 4 + 128 )
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// try to make sure that the raw data structs are packed,
// so that structures can be read/written directly using fread()/fwrite()

#pragma pack(push, 1)

/* MD2 header */
struct s_id_md2_header
{
    int ident;
    int version;

    int skinwidth;
    int skinheight;

    int framesize;

    int num_skins;
    int num_vertices;
    int num_st;
    int num_tris;
    int num_glcmds;
    int num_frames;

    int size_glcmds;

    int offset_skins;
    int offset_st;
    int offset_tris;
    int offset_frames;
    int offset_glcmds;
    int offset_end;
} SET_PACKED();
typedef struct s_id_md2_header id_md2_header_t;

/* Texture name */
struct s_id_md2_skin
{
    char name[64];
} SET_PACKED();
typedef struct s_id_md2_skin id_md2_skin_t;

/* Texture coords */
struct s_id_md2_texcoord
{
    short s;
    short t;
} SET_PACKED();
typedef struct s_id_md2_texcoord id_md2_texcoord_t;

/* Triangle info */
struct s_id_md2_triangle
{
    unsigned short vertex[3];
    unsigned short st[3];
} SET_PACKED();
typedef struct s_id_md2_triangle id_md2_triangle_t;

/* Compressed vertex */
struct s_id_md2_vertex
{
    unsigned char v[3];
    unsigned char normalIndex;
} SET_PACKED();
typedef struct s_id_md2_vertex id_md2_vertex_t;

/* Model frame */
struct s_id_md2_frame
{
    float            scale[3];
    float            translate[3];
    char             name[16];
    id_md2_vertex_t *verts;
} SET_PACKED();
typedef struct s_id_md2_frame id_md2_frame_t;

/* GL command packet */
struct s_id_glcmd_packed
{
    float s;
    float t;
    int index;
} SET_PACKED();
typedef struct s_id_glcmd_packed id_glcmd_packed_t;

/* MD2 model structure */
struct s_id_md2_model
{
    id_md2_header_t    header;

    id_md2_skin_t     *skins;
    id_md2_texcoord_t *texcoords;
    id_md2_triangle_t *triangles;
    id_md2_frame_t    *frames;
    int               *glcmds;
    GLuint             tex_id;
} SET_PACKED();
typedef struct s_id_md2_model id_md2_model_t;

#pragma pack(pop)



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern float kid_md2_normals[MD2_MAX_NORMALS][3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// functions to load the packed data structures directly from a file
// only works with little endian machines
id_md2_model_t * id_md2_load (const char *filename, id_md2_model_t * mdl);
void                    id_md2_free ( id_md2_model_t * mdl );

#define ID_MD2_H
