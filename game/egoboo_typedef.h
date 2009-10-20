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

/// @file egoboo_typedef.h
/// @details some basic types that are used throughout the game code.

#include "egoboo_config.h"
#include <SDL_types.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if defined(__cplusplus)
#    define EGOBOO_NEW( TYPE ) new TYPE
#    define EGOBOO_NEW_ARY( TYPE, COUNT ) new TYPE [ COUNT ]
#    define EGOBOO_DELETE(PTR) if(NULL != PTR) { delete PTR; PTR = NULL; }
#    define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { delete [] PTR; PTR = NULL; }
#else
#    define EGOBOO_NEW( TYPE ) (TYPE *)calloc(1, sizeof(TYPE))
#    define EGOBOO_NEW_ARY( TYPE, COUNT ) (TYPE *)calloc(COUNT, sizeof(TYPE))
#    define EGOBOO_DELETE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
#    define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// a template-like declaration of a list that tracks free elements

#define ACCESS_TYPE_NONE

#define DEFINE_LIST_TYPE(TYPE, NAME, COUNT) \
    struct s_list__##TYPE__##NAME                 \
    {                                          \
        Uint32 used_count;                     \
        int    free_count;                     \
        int    used_ref[COUNT];                \
        int    free_ref[COUNT];                \
        TYPE   lst[COUNT];                     \
    }

#define DEFINE_LIST_EXTERN(TYPE, NAME, COUNT)   \
    DEFINE_LIST_TYPE(TYPE, NAME, COUNT);        \
    extern struct s_list__##TYPE__##NAME NAME

#define DEFINE_LIST_STATIC(TYPE, NAME, COUNT)   \
    DEFINE_LIST_TYPE(TYPE, NAME, COUNT)

#define DECLARE_LIST(ACCESS,TYPE,NAME) ACCESS struct s_list__##TYPE__##NAME NAME = {0, 0}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// a template-like declaration of a list that tracks free elements

#define DEFINE_STACK_TYPE(TYPE, NAME, COUNT) \
    struct s_stack__##TYPE__##NAME           \
    {                                        \
        int  count;                          \
        TYPE lst[COUNT];                     \
    }

#define DEFINE_STACK_EXTERN(TYPE, NAME, COUNT) \
    DEFINE_STACK_TYPE(TYPE, NAME, COUNT);       \
    extern struct s_stack__##TYPE__##NAME NAME

#define DEFINE_STACK_STATIC(TYPE, NAME, COUNT) \
    DEFINE_STACK_TYPE(TYPE, NAME, COUNT)

#define DECLARE_STACK(ACCESS,TYPE,NAME) ACCESS struct s_stack__##TYPE__##NAME NAME = {0}

//--------------------------------------------------------------------------------------------
/// BOOLEAN
typedef char bool_t;
enum
{
    btrue = ( 1 == 1 ),
    bfalse = ( !btrue )
};

//--------------------------------------------------------------------------------------------
/// special return values
enum e_egoboo_rv
{
    rv_error   = -1,
    rv_fail    = bfalse,
    rv_success = btrue
};

typedef enum e_egoboo_rv egoboo_rv;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// basic vector types

typedef float vec2f_t[2];
typedef float vec3f_t[3];
typedef float vec4f_t[4];

typedef double vec2d_t[2];
typedef double vec3d_t[3];
typedef double vec4d_t[4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// RECTANGLE
typedef struct s_irect
{
    int left;
    int right;
    int top;
    int bottom;
} irect_t;

bool_t irect_point_inside( irect_t * prect, int   ix, int   iy );

typedef struct s_frect
{
    float left;
    float right;
    float top;
    float bottom;
} frect_t;

bool_t frect_point_inside( frect_t * prect, float fx, float fy );

//--------------------------------------------------------------------------------------------
// PAIR AND RANGE

// Specifies a value between "base" and "base + rand"
struct s_pair
{
    int base, rand;
};
typedef struct s_pair IPair;

// Specifies a value from "from" to "to"
struct s_range
{
    float from, to;
};
typedef struct s_range FRange;

void pair_to_range( IPair pair, FRange * prange );
void range_to_pair( FRange range, IPair * ppair );

void ints_to_range( int base, int rand, FRange * prange );
void floats_to_pair( float vmin, float vmax, IPair * ppair );

//--------------------------------------------------------------------------------------------
/// some basic data that all egoboo profiles should have
#define  EGO_PROFILE_STUFF \
    bool_t         loaded;      /* Does it exist? */ \
    STRING         name

//--------------------------------------------------------------------------------------------
/// IDSZ
typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(C0,C1,C2,C3)     \
    ((IDSZ)(                       \
        ((((C0)-'A')&0x1F) << 15) |       \
        ((((C1)-'A')&0x1F) << 10) |       \
        ((((C2)-'A')&0x1F) <<  5) |       \
        ((((C3)-'A')&0x1F) <<  0)         \
     ))
#endif

#define IDSZ_NONE            MAKE_IDSZ('N','O','N','E')       ///< [NONE]
#define IDSZ_BOOK            MAKE_IDSZ('B','O','O','K')       ///< [BOOK]

const char * undo_idsz( IDSZ idsz );

//--------------------------------------------------------------------------------------------
/// STRING
typedef char STRING[256];

//--------------------------------------------------------------------------------------------
/// FAST CONVERSIONS
#define FP8_TO_FLOAT(V1)   ( (float)(V1) * INV_0100 )
#define FLOAT_TO_FP8(V1)   ( (Uint32)((V1) * (float)(0x0100) ) )
#define FP8_TO_INT(V1)     ( (V1) >> 8 )                      ///< fast version of V1 / 256
#define INT_TO_FP8(V1)     ( (V1) << 8 )                      ///< fast version of V1 * 256
#define FP8_MUL(V1, V2)    ( ((V1)*(V2)) >> 8 )
#define FP8_DIV(V1, V2)    ( ((V1)<<8) / (V2) )

#define FF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FF )

#define FFFF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FFFF )
#define FLOAT_TO_FFFF( V1 )  ( ((V1) * 0xFFFF) )

#define FLOAT_TO_FP16( V1 )  ( (Uint32)((V1) * 0x00010000) )

#define CLIP_TO_08BITS( V1 )  ( (V1) & 0xFF       )
#define CLIP_TO_16BITS( V1 )  ( (V1) & 0xFFFF     )
#define CLIP_TO_24BITS( V1 )  ( (V1) & 0xFFFFFF   )
#define CLIP_TO_32BITS( V1 )  ( (V1) & 0xFFFFFFFF )

//--------------------------------------------------------------------------------------------
/// List of the methods an AI can use to obtain a target
typedef enum target_type
{
    TARGET_ENEMY = 0,
    TARGET_FRIEND,
    TARGET_ALL,
    TARGET_NONE
} TARGET_TYPE;

//--------------------------------------------------------------------------------------------
/// a hash type for "efficiently" storing data
struct s_hash_node
{
    struct s_hash_node * next;
    void * data;
};
typedef struct s_hash_node hash_node_t;

//--------------------------------------------------------------------------------------------
struct s_hash_list
{
    int            allocated;
    int         *  subcount;
    hash_node_t ** sublist;
};
typedef struct s_hash_list hash_list_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_create(void * data);
bool_t        hash_node_destroy(hash_node_t **);
hash_node_t * hash_node_insert_after (hash_node_t lst[], hash_node_t * n);
hash_node_t * hash_node_insert_before(hash_node_t lst[], hash_node_t * n);
hash_node_t * hash_node_remove_after (hash_node_t lst[]);
hash_node_t * hash_node_remove       (hash_node_t lst[]);

hash_list_t * hash_list_create(int size);
bool_t        hash_list_destroy(hash_list_t **);

hash_node_t * hash_node_ctor(hash_node_t * n, void * data);
hash_list_t * hash_list_ctor(hash_list_t * lst, int size);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// axis aligned bounding box
struct s_aabb
{
    float mins[3];
    float maxs[3];
};
typedef struct s_aabb aabb_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The various axes for the octagonal bounding box
enum e_octagonal_axes
{
    OCT_X, OCT_Y, OCT_Z, OCT_XY, OCT_YX, OCT_COUNT
};

/// octagonal bounding box
struct s_oct_bb
{
    float mins[OCT_COUNT];
    float maxs[OCT_COUNT];
};
typedef struct s_oct_bb oct_bb_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_latch
{
    float          x;         ///< the x input
    float          y;         ///< the y input
    Uint32         b;         ///< the button bits
};

typedef struct s_latch latch_t;

void latch_init( latch_t * platch );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some basic data that all egoboo objects should have

/// A variable to hold the object guid counter
extern Uint32 ego_object_guid;

/// The possible states of an ego_object_base_t object
enum e_ego_object_state
{
    ego_object_invalid = 0,
    ego_object_pre_active,               ///< in the process of being activated
    ego_object_active,                   ///< fully activated
    ego_object_waiting,                  ///< waiting to be terminated
    ego_object_terminated                ///< fully terminated
};

/// The data that is "inherited" by every egoboo object.
struct s_ego_object_base
{
    STRING         _name;     ///< what is its "_name"
    int            index;     ///< what is the index position in the object list?
    bool_t         allocated; ///< Does it exist?
    int            state;     ///< what state is it in?
    Uint32         guid;      ///< a globally unique identifier
};

typedef struct s_ego_object_base ego_object_base_t;

/// Mark a ego_object_base_t object as being allocated
#define EGO_OBJECT_ALLOCATE( PDATA, INDEX ) \
    if( NULL != PDATA ) \
    { \
        (PDATA)->obj_base.allocated = btrue; \
        (PDATA)->obj_base.index     = INDEX; \
        (PDATA)->obj_base.state     = ego_object_pre_active; \
        (PDATA)->obj_base.guid      = ego_object_guid++; \
    }

/// Turn on an ego_object_base_t object
#define EGO_OBJECT_ACTIVATE( PDATA, NAME ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    { \
        strncpy( (PDATA)->obj_base._name, NAME, SDL_arraysize((PDATA)->obj_base._name) ); \
        (PDATA)->obj_base.state  = ego_object_active; \
    }

/// Begin turning off an ego_object_base_t object
#define EGO_OBJECT_REQUST_TERMINATE( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    { \
        (PDATA)->obj_base.state = ego_object_waiting; \
    }

/// Completely turn off an ego_object_base_t object and mark it as no longer allocated
#define EGO_OBJECT_TERMINATE( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    { \
        (PDATA)->obj_base.allocated = bfalse; \
        (PDATA)->obj_base.state     = ego_object_terminated; \
    }

/// Is the object allocated?
#define ALLOCATED_OBJ( POBJ )   ( (NULL != (POBJ)) && ( (POBJ)->allocated ) && (ego_object_invalid != (POBJ)->state) )

/// Is the object "on"
#define ACTIVE_OBJ( POBJ )      ( ALLOCATED_OBJ( POBJ ) && (ego_object_active == (POBJ)->state) )

/// Is the object waiting to "die"
#define WAITING_OBJ( POBJ )     ( ALLOCATED_OBJ( POBJ ) && (ego_object_waiting == (POBJ)->state) )

/// Has the object been marked as terminated
#define TERMINATED_OBJ( POBJ )  ( (NULL != (POBJ)) && (ego_object_terminated == (POBJ)->state) )

/// Grab a pointer to the ego_object_base_t of an object that "inherits" this data
#define OBJ_GET_PBASE( PBLAH )          ( (NULL == (PBLAH)) ? NULL : &((PBLAH)->obj_base) )

/// Grab the index value of object that "inherits" from ego_object_base_t
#define GET_INDEX( PBLAH, FAIL_VALUE )  ( (NULL == (PBLAH) || !ALLOCATED_OBJ( OBJ_GET_PBASE( (PBLAH) ) ) ) ? FAIL_VALUE : (PBLAH)->obj_base.index )


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define Egoboo_egobootypedef_h
