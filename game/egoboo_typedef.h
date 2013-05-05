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
/// @details base type definitions and config options

#include "../egolib/typedef.h"

#include "egoboo_config.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// definition of the c-type reference

#   define C_DECLARE_REF( NAME ) typedef REF_T NAME

// define the c implementation always
#   define C_REF_TO_INT(X) ((REF_T)(X))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple array

#   define C_DECLARE_T_ARY(TYPE, NAME, COUNT)  TYPE   NAME[COUNT]

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple list structure that tracks free elements

#   define ACCESS_TYPE_NONE

#   define INVALID_UPDATE_GUID ((unsigned)(~((unsigned)0)))

#   define C_DEFINE_LIST_TYPE(TYPE, NAME, COUNT) \
    struct s_c_list__##TYPE__##NAME           \
    {                                         \
        unsigned update_guid;                 \
        int      used_count;                  \
        int      free_count;                  \
        size_t   used_ref[COUNT];             \
        size_t   free_ref[COUNT];             \
        C_DECLARE_T_ARY(TYPE, lst, COUNT);    \
    }

#   define C_DECLARE_LIST_EXTERN(TYPE, NAME, COUNT) \
    C_DEFINE_LIST_TYPE(TYPE, NAME, COUNT);          \
    void   NAME##_ctor( void );                     \
    void   NAME##_dtor( void );                     \
    ego_bool NAME##_push_used( const REF_T );         \
    TYPE * NAME##_get_ptr( const size_t );          \
    extern struct s_c_list__##TYPE__##NAME NAME

#   define C_INSTANTIATE_LIST_STATIC(TYPE, NAME, COUNT) \
    C_DEFINE_LIST_TYPE(TYPE, NAME, COUNT);        \
    static struct s_c_list__##TYPE__##NAME NAME = {INVALID_UPDATE_GUID, 0, 0}

#   define C_INSTANTIATE_LIST(ACCESS,TYPE,NAME, COUNT) \
    ACCESS struct s_c_list__##TYPE__##NAME NAME = {INVALID_UPDATE_GUID, 0, 0}

#   define C_IMPLEMENT_LIST(TYPE, NAME, COUNT)          \
    static int     NAME##_find_free_ref( const REF_T ); \
    static ego_bool  NAME##_push_free( const REF_T );     \
    static size_t  NAME##_pop_free( const int );        \
    static int     NAME##_find_used_ref( const REF_T ); \
    static size_t  NAME##_pop_used( const int );        \
    TYPE * NAME##_get_ptr( const size_t index )   { return LAMBDA(index >= COUNT, NULL, NAME.lst + index); }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple stack structure

#   define C_DEFINE_STACK_TYPE(TYPE, NAME, COUNT) \
    struct s_c_stack__##TYPE__##NAME           \
    {                                          \
        unsigned update_guid;                  \
        int  count;                            \
        C_DECLARE_T_ARY(TYPE, lst, COUNT);     \
    }

#   define C_DECLARE_STACK_EXTERN(TYPE, NAME, COUNT) \
    C_DEFINE_STACK_TYPE(TYPE, NAME, COUNT);       \
    TYPE * NAME##_get_ptr( size_t );                 \
    extern struct s_c_stack__##TYPE__##NAME NAME

#   define C_INSTANTIATE_STACK_STATIC(TYPE, NAME, COUNT) \
    C_DEFINE_STACK_TYPE(TYPE, NAME, COUNT);       \
    static struct s_c_stack__##TYPE__##NAME NAME = {0}

#   define C_INSTANTIATE_STACK(ACCESS, TYPE, NAME, COUNT) \
    ACCESS struct s_c_stack__##TYPE__##NAME NAME = {INVALID_UPDATE_GUID, 0}

#   define C_IMPLEMENT_STACK(TYPE, NAME, COUNT)  \
    TYPE * NAME##_get_ptr( size_t index )   { return (index >= COUNT) ? NULL : NAME.lst + index; }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a template-like declaration of a dynamically allocated array
#   define DECLARE_DYNAMIC_ARY(ARY_T, ELEM_T) \
    struct s_DYNAMIC_ARY_##ARY_T \
    { \
        size_t alloc; \
        int    top;  \
        ELEM_T * ary; \
    }; \
    typedef struct s_DYNAMIC_ARY_##ARY_T ARY_T##_t; \
    \
    ARY_T##_t * ARY_T##_ctor( ARY_T##_t * pary, size_t sz ); \
    ARY_T##_t * ARY_T##_dtor( ARY_T##_t * pary ); \
    ego_bool   ARY_T##_alloc( ARY_T##_t * pary, size_t sz ); \
    ego_bool   ARY_T##_free( ARY_T##_t * pary ); \
    void        ARY_T##_clear( ARY_T##_t * pary ); \
    size_t      ARY_T##_get_top( const ARY_T##_t * pary ); \
    size_t      ARY_T##_get_size( const ARY_T##_t * pary ); \
    ELEM_T *    ARY_T##_pop_back( ARY_T##_t * pary ); \
    ego_bool   ARY_T##_push_back( ARY_T##_t * pary , ELEM_T val );

#   define DYNAMIC_ARY_INIT_VALS {0,0,NULL}

#   define INSTANTIATE_DYNAMIC_ARY(ARY_T, NAME) ARY_T##_t NAME = DYNAMIC_ARY_INIT_VALS;

#   define IMPLEMENT_DYNAMIC_ARY(ARY_T, ELEM_T) \
    ego_bool      ARY_T##_alloc( ARY_T##_t * pary, size_t sz )   { if(NULL == pary) return ego_false; ARY_T##_free( pary ); pary->ary = EGOBOO_NEW_ARY( ELEM_T, sz );  pary->alloc = (NULL == pary->ary) ? 0 : sz; return ego_true; } \
    ego_bool      ARY_T##_free(ARY_T##_t * pary )                { if(NULL == pary) return ego_false; EGOBOO_DELETE_ARY(pary->ary); pary->alloc = 0; pary->top = 0; return ego_true; } \
    ARY_T##_t * ARY_T##_ctor(ARY_T##_t * pary, size_t sz)      { if(NULL == pary) return NULL;   BLANK_STRUCT_PTR( pary ) if( !ARY_T##_alloc(pary, sz) ) return NULL; return pary; } \
    ARY_T##_t * ARY_T##_dtor(ARY_T##_t * pary )                { if(NULL == pary) return NULL;   ARY_T##_free(pary); BLANK_STRUCT_PTR( pary ) return pary; } \
    \
    void   ARY_T##_clear( ARY_T##_t * pary )                   { if(NULL != pary) pary->top = 0; } \
    size_t ARY_T##_get_top( const ARY_T##_t * pary )           { return (NULL == pary->ary) ? 0 : pary->top; } \
    size_t ARY_T##_get_size( const ARY_T##_t * pary )          { return (NULL == pary->ary) ? 0 : pary->alloc; } \
    \
    ELEM_T * ARY_T##_pop_back( ARY_T##_t * pary )              { if( NULL == pary || pary->top < 1 ) return NULL; --pary->top; return &(pary->ary[pary->top]); } \
    ego_bool   ARY_T##_push_back( ARY_T##_t * pary, ELEM_T val ) { ego_bool retval = ego_false; if( NULL == pary ) return ego_false; if (pary->top >= 0 && (size_t)pary->top < pary->alloc) { pary->ary[pary->top] = val; pary->top++; retval = ego_true; } return retval; }

#   define DYNAMIC_ARY_INVALID_RAW(PARY) ( (0 == (PARY)->alloc) || ((PARY)->top < 0) || ((size_t)(PARY)->top >= (PARY)->alloc) )
#   define DYNAMIC_ARY_INVALID(PARY) ( (NULL == (PARY)) || DYNAMIC_ARY_INVALID_RAW(PARY) )

#   define DYNAMIC_ARY_VALID_RAW(PARY) ( ((PARY)->alloc > 0) && ((PARY)->top >= 0) && ((size_t)(PARY)->top < (PARY)->alloc) )
#   define DYNAMIC_ARY_VALID(PARY) ( (NULL != (PARY)) && DYNAMIC_ARY_VALID_RAW(PARY) )

// a NULL, invalid, or empty list are all "empty"
#   define DYNAMIC_ARY_HAS_ELEMENTS_RAW(PARY) ( ((PARY)->alloc > 0) && ((PARY)->top > 0) && (((size_t)(PARY)->top) < (PARY)->alloc) )
#   define DYNAMIC_ARY_HAS_ELEMENTS(PARY) ( (NULL != (PARY)) && DYNAMIC_ARY_HAS_ELEMENTS_RAW(PARY) )

// only valid lists can be full
// avoid subtraction from unsigned values
#   define DYNAMIC_ARY_CAN_ADD_ELEMENTS_RAW(PARY) ( ((PARY)->alloc > 0) && ((PARY)->top >= 0) && (((size_t)(PARY)->top) + 1 < (PARY)->alloc) )
#   define DYNAMIC_ARY_CAN_ADD_ELEMENTS(PARY) ( (NULL != (PARY)) && DYNAMIC_ARY_CAN_ADD_ELEMENTS_RAW(PARY) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a template-like declaration of a statically allocated array

#   define DECLARE_STATIC_ARY_TYPE(ARY_T, ELEM_T, SIZE) \
    struct s_STATIC_ARY_##ARY_T \
    { \
        int    count;     \
        ELEM_T ary[SIZE]; \
    }; \
    typedef ELEM_T ARY_T##ELEM_t; \
    typedef struct s_STATIC_ARY_##ARY_T ARY_T##_t

#   define STATIC_ARY_INIT_VALS {0}

#   define DECLARE_EXTERN_STATIC_ARY(ARY_T, NAME)       \
    ARY_T##ELEM_t * ARY_T##_get_ptr( ARY_T##_t *, size_t ); \
    extern ARY_T##_t NAME

#   define INSTANTIATE_STATIC_ARY(ARY_T, NAME) \
    ARY_T##_t NAME = STATIC_ARY_INIT_VALS;

#   define IMPLEMENT_STATIC_ARY(ARY_T, SIZE) \
    ARY_T##ELEM_t * ARY_T##_get_ptr( ARY_T##_t * pary, size_t index ) { if(NULL == pary) return NULL; return ( index >= SIZE ) ? NULL : pary->ary + index; }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// implementation of forward declaration of references

#   define REF_TO_INT(X)  C_REF_TO_INT(X)

#   define DECLARE_T_ARY(TYPE, NAME, COUNT)              C_DECLARE_T_ARY(TYPE, NAME, COUNT)

#   define DECLARE_LIST_EXTERN(TYPE, NAME, COUNT)        C_DECLARE_LIST_EXTERN(TYPE, NAME, COUNT)
#   define INSTANTIATE_LIST_STATIC(TYPE, NAME, COUNT)    C_INSTANTIATE_LIST_STATIC(TYPE, NAME, COUNT)
#   define INSTANTIATE_LIST(ACCESS, TYPE, NAME, COUNT)   C_INSTANTIATE_LIST(ACCESS, TYPE, NAME, COUNT)
#   define IMPLEMENT_LIST(TYPE, NAME, COUNT)             C_IMPLEMENT_LIST(TYPE, NAME, COUNT)

#   define DECLARE_STACK_EXTERN(TYPE, NAME, COUNT)      C_DECLARE_STACK_EXTERN(TYPE, NAME, COUNT)
#   define INSTANTIATE_STACK_STATIC(TYPE, NAME, COUNT)  C_INSTANTIATE_STACK_STATIC(TYPE, NAME, COUNT)
#   define INSTANTIATE_STACK(ACCESS, TYPE, NAME, COUNT) C_INSTANTIATE_STACK(ACCESS, TYPE, NAME, COUNT)
#   define IMPLEMENT_STACK(TYPE, NAME, COUNT)           C_IMPLEMENT_STACK(TYPE, NAME, COUNT)

// use an underscore to force the c implementation
#   define _DECLARE_T_ARY(TYPE, NAME, COUNT)              C_DECLARE_T_ARY(TYPE, NAME, COUNT)

#   define _DECLARE_LIST_EXTERN(TYPE, NAME, COUNT)        C_DECLARE_LIST_EXTERN(TYPE, NAME, COUNT)
#   define _INSTANTIATE_LIST_STATIC(TYPE,NAME, COUNT)     C_INSTANTIATE_LIST_STATIC(TYPE,NAME, COUNT)
#   define _INSTANTIATE_LIST(ACCESS,TYPE,NAME, COUNT)     C_INSTANTIATE_LIST(ACCESS,TYPE,NAME, COUNT)

#   define _DECLARE_STACK_EXTERN(TYPE, NAME, COUNT)      C_DECLARE_STACK_EXTERN(TYPE, NAME, COUNT)
#   define _INSTANTIATE_STACK_STATIC(TYPE, NAME, COUNT)  C_INSTANTIATE_STACK_STATIC(TYPE, NAME, COUNT)
#   define _INSTANTIATE_STACK(ACCESS, TYPE, NAME, COUNT) C_INSTANTIATE_STACK(ACCESS, TYPE, NAME, COUNT)

    C_DECLARE_REF( CAP_REF );
    C_DECLARE_REF( CHR_REF );
    C_DECLARE_REF( TEAM_REF );
    C_DECLARE_REF( EVE_REF );
    C_DECLARE_REF( ENC_REF );
    C_DECLARE_REF( MAD_REF );
    C_DECLARE_REF( PLA_REF );
    C_DECLARE_REF( PIP_REF );
    C_DECLARE_REF( PRT_REF );
    C_DECLARE_REF( PASS_REF );
    C_DECLARE_REF( SHOP_REF );
    C_DECLARE_REF( PRO_REF );
    C_DECLARE_REF( TX_REF );
    C_DECLARE_REF( MNU_TX_REF );
    C_DECLARE_REF( BBOARD_REF );
    C_DECLARE_REF( LOOP_REF );
    C_DECLARE_REF( MOD_REF );
    C_DECLARE_REF( TREQ_REF );
    C_DECLARE_REF( MOD_REF_REF );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// forward declaration of standard dynamic array types

    DECLARE_DYNAMIC_ARY( char_ary,   char )
    DECLARE_DYNAMIC_ARY( short_ary,  short )
    DECLARE_DYNAMIC_ARY( int_ary,    int )
    DECLARE_DYNAMIC_ARY( float_ary,  float )
    DECLARE_DYNAMIC_ARY( double_ary, double )
