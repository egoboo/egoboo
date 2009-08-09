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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - egoboo_typedef.h
 * Defines some basic types that are used throughout the game code.
 */

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
// a template-like declaration of a list that tracks free elements

#define DEFINE_LIST(ACCESS, TYPE, NAME, COUNT) \
    struct s_list__##TYPE__##NAME                 \
    {                                          \
        int  used_count;                       \
        int  free_count;                       \
        int  used_ref[COUNT];                  \
        int  free_ref[COUNT];                  \
        TYPE lst[COUNT];                       \
    };                                         \
    ACCESS struct s_list__##TYPE__##NAME NAME;

#define DECLARE_LIST(TYPE,NAME) struct s_list__##TYPE__##NAME NAME = {0, 0};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a template-like declaration of a list that tracks free elements

#define DEFINE_STACK(ACCESS, TYPE, NAME, COUNT) \
    struct s_stack__##TYPE__##NAME                 \
    {                                           \
        int  count;                             \
        TYPE lst[COUNT];                        \
    };                                          \
    ACCESS struct s_stack__##TYPE__##NAME NAME;

#define DECLARE_STACK(TYPE,NAME) struct s_stack__##TYPE__##NAME NAME = {0};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// RECTANGLE
typedef struct s_rect
{
    Sint32 left;
    Sint32 right;
    Sint32 top;
    Sint32 bottom;
} rect_t;

//--------------------------------------------------------------------------------------------
// PAIR AND RANGE

struct s_pair
{
    int base, rand;
};
typedef struct s_pair IPair;

struct s_range
{
    float from, to;
};
typedef struct s_range FRange;

//--------------------------------------------------------------------------------------------
// BOOLEAN
typedef char bool_t;
enum
{
    btrue = ( 1 == 1 ),
    bfalse = ( !btrue )
};

//--------------------------------------------------------------------------------------------
// some basic data that all egoboo objects should have
#define  EGO_OBJECT_STUFF \
    bool_t         on;      /* Does it exist? */ \
    bool_t         onold;   /* Network fix    */ \
    STRING         name;

//--------------------------------------------------------------------------------------------
// some basic data that all egoboo profiles should have
#define  EGO_PROFILE_STUFF \
    bool_t         loaded;      /* Does it exist? */ \
    STRING         name;

//--------------------------------------------------------------------------------------------
// IDSZ
typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(C0,C1,C2,C3) \
    ((IDSZ)(                       \
                                   (((C0)-'A') << 15) |       \
                                   (((C1)-'A') << 10) |       \
                                   (((C2)-'A') <<  5) |       \
                                   (((C3)-'A') <<  0)         \
           ))
#endif

#define IDSZ_NONE            MAKE_IDSZ('N','O','N','E' )       // [NONE]
#define IDSZ_BOOK            MAKE_IDSZ('B','O','O','K' )       // [BOOK]

//--------------------------------------------------------------------------------------------
// STRING
typedef char STRING[256];

//--------------------------------------------------------------------------------------------
// FAST CONVERSIONS
#define FP8_TO_FLOAT(XX)   ( (float)(XX) * INV_0100 )
#define FLOAT_TO_FP8(XX)   ( (Uint32)((XX) * (float)(0x0100) )
#define FP8_TO_INT(XX)     ( (XX) >> 8 )                      // fast version of XX / 256
#define INT_TO_FP8(XX)     ( (XX) << 8 )                      // fast version of XX * 256
#define FP8_MUL(XX, YY)    ( ((XX)*(YY)) >> 8 )
#define FP8_DIV(XX, YY)    ( ((XX)<<8) / (YY) )

// "fast" multiplication for the case where 0xFF == 1.00
#define FF_MUL(XX, YY)     ( ( 0 == (XX) || 0 == (YY) ) ? 0 : ( ( ((XX)+1) * ((YY)+1) ) >> 8 ) )
#define FF_TO_FLOAT( XX )  ( (float)(XX) * INV_FF )

//--------------------------------------------------------------------------------------------
// AI targeting
typedef enum target_type
{
    TARGET_ENEMY = 0,
    TARGET_FRIEND,
    TARGET_ALL,
    TARGET_NONE
} TARGET_TYPE;

//--------------------------------------------------------------------------------------------
// a hash type for "efficiently" storing data
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
// axis aligned bounding box
struct s_aabb
{
    float mins[3];
    float maxs[3];
};
typedef struct s_aabb aabb_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define Egoboo_egobootypedef_h
