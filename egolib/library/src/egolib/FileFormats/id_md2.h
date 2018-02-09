#pragma once

/// @file egolib/FileFormats/id_md2.h
/// @details Md2 model file loading structures & constants.

#include "egolib/Extensions/ogl_extensions.h"
#include <string>

//--------------------------------------------------------------------------------------------
// gcc does not properly recognize #pragma pack statements

#if !defined(SET_PACKED)
#    if defined(__GNUC__)
#        define SET_PACKED() __attribute__ ((__packed__))
#    else
#        define SET_PACKED()
#    endif
#endif
//--------------------------------------------------------------------------------------------

/// Constants describing the standard md2 file
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
// try to make sure that the raw data structs are packed,
// so that structures can be read/written directly using fread()/fwrite()

#pragma pack(push, 1)

    /* MD2 header */
    struct id_md2_header_t
    {
        int32_t ident;
        int32_t version;

        int32_t skinwidth;
        int32_t skinheight;
        int32_t framesize;

        int32_t num_skins;
        int32_t num_vertices;
        int32_t num_st;
        int32_t num_tris;
        int32_t size_glcmds;
        int32_t num_frames;

        int32_t offset_skins;
        int32_t offset_st;
        int32_t offset_tris;
        int32_t offset_frames;
        int32_t offset_glcmds;
        int32_t offset_end;

    } SET_PACKED();

    /* Texture name */
    struct id_md2_skin_t
    {
        char name[64];
    } SET_PACKED();

    /* Texture coords */
    struct id_md2_texcoord_t
    {
        int16_t s;
        int16_t t;
    } SET_PACKED();

    /* Triangle info */
    struct id_md2_triangle_t
    {
        uint16_t vertex[3];
        uint16_t st[3];
    } SET_PACKED();

    /* Compressed vertex */
    struct id_md2_vertex_t
    {
        uint8_t v[3];
        uint8_t normalIndex;
    } SET_PACKED();

    /* Model frame */
    struct id_md2_frame_header_t
    {
        float            scale[3];
        float            translate[3];
        char             name[16];
    } SET_PACKED();

    /* Model frame */
    struct id_md2_frame_t
    {
        float            scale[3];
        float            translate[3];
        char             name[16];
        id_md2_vertex_t *verts;
    } SET_PACKED();

    /* GL command packet */
    struct id_glcmd_packed_t
    {
        float s;
        float t;
        int32_t index;
    } SET_PACKED();

    /* MD2 model structure */
    struct id_md2_model_t
    {
        id_md2_header_t    header;

        id_md2_skin_t     *skins;
        id_md2_texcoord_t *texcoords;
        id_md2_triangle_t *triangles;
        id_md2_frame_t    *frames;
        int               *glcmds;
        GLuint             tex_id;
    } SET_PACKED();

#pragma pack(pop)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern float kid_md2_normals[MD2_MAX_NORMALS][3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// functions to load the packed data structures directly from a file
/// only works with little endian machines
    id_md2_model_t * id_md2_load( const std::string& filename, id_md2_model_t * mdl );
    void             id_md2_free( id_md2_model_t * mdl );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
