#pragma once

//--------------------------------------------------------------------------------------------
// A MD2 reader using SDL.
// - reads the data into unpacked structures
// - converts data to the local endian-ness using the SDL RWops

#include <SDL/SDL_types.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_rwops.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_id_glcmd_packed;
    typedef struct s_id_glcmd_packed id_glcmd_packed_t;

    struct s_SDL_md2_header;
    typedef struct s_SDL_md2_header SDL_md2_header_t;

    struct s_SDL_md2_skin;
    typedef struct s_SDL_md2_skin SDL_md2_skin_t;

    struct s_SDL_md2_texcoord;
    typedef struct s_SDL_md2_texcoord SDL_md2_texcoord_t;

    struct s_SDL_md2_triangle;
    typedef struct s_SDL_md2_triangle SDL_md2_triangle_t;

    struct s_SDL_md2_vertex;
    typedef struct s_SDL_md2_vertex SDL_md2_vertex_t;

    struct s_SDL_md2_frame;
    typedef struct s_SDL_md2_frame SDL_md2_frame_t;

    struct s_SDL_md2_model;
    typedef struct s_SDL_md2_model SDL_md2_model_t;

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

// gcc does not respect #pragma pack statements

#if !defined(SET_PACKED)
#    if defined(__GNUC__)
#        define SET_PACKED() __attribute__ ((__packed__))
#    else
#        define SET_PACKED()
#    endif
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    typedef float  md2_vec3f_t[3];
    typedef Uint8  md2_vec3uc_t[3];
    typedef Uint16 md2_vec3us_t[3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// UNPACKED versions of ID's md2 data structures, so they are optimized for access

    /* MD2 header */
    struct s_SDL_md2_header
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
        int size_glcmds;
        int num_frames;

        int offset_skins;
        int offset_st;
        int offset_tris;
        int offset_frames;
        int offset_glcmds;
        int offset_end;

        int num_glcmds;
    };

    /* Texture name */
    struct s_SDL_md2_skin
    {
        char name[64];
    };

    /* Texture coords */
    struct s_SDL_md2_texcoord
    {
        short s;
        short t;
    };

    /* Triangle info */
    struct s_SDL_md2_triangle
    {
        md2_vec3us_t vertex;
        md2_vec3us_t st;
    };

    /* Compressed vertex */
    struct s_SDL_md2_vertex
    {
        md2_vec3uc_t v;
        Uint8    normalIndex;
    };

    /* Model frame */
    struct s_SDL_md2_frame
    {
        md2_vec3f_t          scale;
        md2_vec3f_t          translate;
        char             name[16];
        SDL_md2_vertex_t *verts;
    };

    /* MD2 model structure */
    struct s_SDL_md2_model
    {
        SDL_md2_header_t    header;

        SDL_md2_skin_t     *skins;
        SDL_md2_texcoord_t *texcoords;
        SDL_md2_triangle_t *triangles;
        SDL_md2_frame_t    *frames;
        Sint32             *glcmds;
        GLuint              tex_id;
    };

#pragma pack(push, 1)
    /* GL command packet */
    struct s_id_glcmd_packed
    {
        float s;
        float t;
        int index;
    } SET_PACKED();
#pragma pack(pop)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern float kid_md2_normals[MD2_MAX_NORMALS][3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define SDL_md2_load( FNAME, PMODEL ) SDL_md2_load_RW( SDL_RWFromFile (FNAME, "rb"), PMODEL )

// functions to load the unpacked data structures
// works for both big and little endian machines
    SDL_md2_model_t * SDL_md2_new( SDL_md2_model_t * mdl );
    void              SDL_md2_free( SDL_md2_model_t * mdl );
// SDL_md2_model_t * SDL_md2_load    ( const char *filename, SDL_md2_model_t * mdl );
    SDL_md2_model_t * SDL_md2_load_RW( SDL_RWops * rw,       SDL_md2_model_t * mdl );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define sdl_md2_h
