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
// Rectangle types

struct s_ego_irect
{
    int xmin, ymin;
    int xmax, ymax;
};
typedef struct s_ego_irect ego_irect_t;

struct s_ego_frect
{
    float xmin, ymin;
    float xmax, ymax;
};
typedef struct s_ego_frect ego_frect_t;

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
/// List of the methods an AI can use to obtain a target
typedef enum target_type
{
    TARGET_ENEMY = 0,
    TARGET_FRIEND,
    TARGET_ALL,
    TARGET_NONE
} TARGET_TYPE;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// The latch used by the input system
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
//--------------------------------------------------------------------------------------------
#define Egoboo_egoboo_typedef_h
