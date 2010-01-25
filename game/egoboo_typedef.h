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
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
/// BOOLEAN

#if defined __cplusplus
#   define bool_t bool
#   define btrue  true
#   define bfalse false
#else
enum e_bool
{
    btrue  = ( 1 == 1 ),
    bfalse = ( !btrue )
};
typedef enum e_bool bool_t;
#endif

//--------------------------------------------------------------------------------------------
// 24.8 fixed point types

typedef Uint32 UFP8_T;
typedef Sint32 SFP8_T;

#define FP8_TO_FLOAT(V1)   ( (float)(V1) * INV_0100 )
#define FLOAT_TO_FP8(V1)   ( (Uint32)((V1) * (float)(0x0100) ) )
#define FP8_TO_INT(V1)     ( (V1) >> 8 )                      ///< fast version of V1 / 256
#define INT_TO_FP8(V1)     ( (V1) << 8 )                      ///< fast version of V1 * 256
#define FP8_MUL(V1, V2)    ( ((V1)*(V2)) >> 8 )               ///< this may overflow if V1 or V2 have non-zero bits in their upper 8 bits
#define FP8_DIV(V1, V2)    ( ((V1)<<8) / (V2) )               ///< this  will fail if V1 has bits in the upper 8 bits

//--------------------------------------------------------------------------------------------
// 16.16 fixed point types

typedef Uint32 UFP16_T;
typedef Sint32 SFP16_T;

#define FLOAT_TO_FP16( V1 )  ( (Uint32)((V1) * 0x00010000) )
#define FP16_TO_FLOAT( V1 )  ( (float )((V1) * 0.0000152587890625f ) )

//--------------------------------------------------------------------------------------------
// References

typedef Uint16 REF_T;

#ifdef __cplusplus
#    define REF_TO_INT(X) ((Uint16)(X))
#else
#    define REF_TO_INT(X) (X)
#endif

//--------------------------------------------------------------------------------------------
// fix for the fact that assert is technically not suppoeted in c++

#if defined(__cplusplus)
#    include <exception>
class egoboo_exception : public std::exception
{
    protected:
        const char * what;
    public:
        egoboo_exception( const char * str ) : what( str ) {};
};
#    define EGOBOO_ASSERT(X) if( !(X) ) { throw new egoboo_exception( #X ); }
#else
#    define EGOBOO_ASSERT(X) assert(X)
#endif

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
    struct s_list__##TYPE__##NAME           \
    {                                       \
        Uint32 used_count;                  \
        int    free_count;                  \
        int    used_ref[COUNT];             \
        int    free_ref[COUNT];             \
        TYPE   lst[COUNT];                  \
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
/// a template-like declaration of an array

#define DEFINE_ARY(ARY_T, ELEM_T) \
    struct s_##ARY_T \
    { \
        size_t alloc; \
        int    top;  \
        ELEM_T * ary; \
    }; \
    typedef struct s_##ARY_T ARY_T##_t; \
    \
    ARY_T##_t * ARY_T##_ctor( ARY_T##_t * pary, size_t sz ); \
    ARY_T##_t * ARY_T##_dtor( ARY_T##_t * pary ); \
    bool_t      ARY_T##_alloc( ARY_T##_t * pary, size_t sz ); \
    bool_t      ARY_T##_free( ARY_T##_t * pary ); \
    void        ARY_T##_clear( ARY_T##_t * pary ); \
    bool_t      ARY_T##_push_back( ARY_T##_t * pary, ELEM_T val ); \
    size_t      ARY_T##_get_top( ARY_T##_t * pary ); \
    size_t      ARY_T##_get_size( ARY_T##_t * pary ); \
    ELEM_T *    ARY_T##_pop_back( ARY_T##_t * pary ); \
    bool_t      ARY_T##_push_back( ARY_T##_t * pary , ELEM_T val );

#define ARY_INIT_VALS {0,0,NULL}

#define DECLARE_ARY(ARY_T, NAME) ARY_T##_t NAME = ARY_INIT_VALS;

#define IMPLEMENT_ARY(ARY_T, ELEM_T) \
    bool_t      ARY_T##_alloc( ARY_T##_t * pary, size_t sz )  { if(NULL == pary) return bfalse; ARY_T##_free( pary ); pary->ary = EGOBOO_NEW_ARY( ELEM_T, sz );  pary->alloc = (NULL == pary->ary) ? 0 : sz; return btrue; } \
    bool_t      ARY_T##_free(ARY_T##_t * pary )            { if(NULL == pary) return bfalse; EGOBOO_DELETE_ARY(pary->ary); pary->alloc = 0; pary->top = 0; return btrue; } \
    ARY_T##_t * ARY_T##_ctor(ARY_T##_t * pary, size_t sz)     { if(NULL == pary) return NULL;   memset(pary, 0, sizeof(*pary)); if( !ARY_T##_alloc(pary, sz) ) return NULL; return pary; } \
    ARY_T##_t * ARY_T##_dtor(ARY_T##_t * pary )               { if(NULL == pary) return NULL;   ARY_T##_free(pary); memset(pary, 0, sizeof(*pary)); return pary; } \
    \
    void   ARY_T##_clear( ARY_T##_t * pary )                  { if(NULL != pary) pary->top = 0; } \
    size_t ARY_T##_get_top( ARY_T##_t * pary )                { return (NULL == pary->ary) ? 0 : pary->top; } \
    size_t ARY_T##_get_size( ARY_T##_t * pary )               { return (NULL == pary->ary) ? 0 : pary->alloc; } \
    \
    ELEM_T * ARY_T##_pop_back( ARY_T##_t * pary )              { if( NULL == pary || pary->top < 1 ) return NULL; --pary->top; return &(pary->ary[pary->top]); } \
    bool_t   ARY_T##_push_back( ARY_T##_t * pary, ELEM_T val ) { bool_t retval = bfalse; if( NULL == pary ) return bfalse; if (pary->top < pary->alloc) { pary->ary[pary->top] = val; pary->top++; retval = btrue; } return retval; }

// define simple type arrays
DEFINE_ARY( char_ary,   char )
DEFINE_ARY( short_ary,  short )
DEFINE_ARY( int_ary,    int )
DEFINE_ARY( float_ary,  float )
DEFINE_ARY( double_ary, double )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define Egoboo_egoboo_typedef_h
