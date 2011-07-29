#pragma once

///
/// @file egolib_frustum.h
/// @brief integrating the basic frustum object into Egoboo algorithms

#include "../egolib/typedef.h"

#include "../egolib/geometry.h"
#include "../egolib/bbox.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

    struct s_oct_bb;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

    struct s_ego_aabb;
    typedef struct s_ego_aabb ego_aabb_t;

    struct s_egolib_frustum;
    typedef struct s_egolib_frustum egolib_frustum_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_ego_aabb
    {
        sphere_t sphere;
        aabb_t   data;
    };

#define EGO_AABB_INIT_VALS                 \
    {                                          \
        SPHERE_INIT_VALS, /*sphere_t sphere */ \
        AABB_INIT_VALS    /*aabb_t   data   */ \
    }

    ego_aabb_t * ego_aabb_ctor( ego_aabb_t * );
    ego_aabb_t * ego_aabb_dtor( ego_aabb_t * );
    bool_t ego_aabb_self_clear( ego_aabb_t * );
    bool_t ego_aabb_is_clear( const ego_aabb_t * pdst );

    bool_t ego_aabb_self_union( ego_aabb_t * pdst, const ego_aabb_t * psrc );
    bool_t ego_aabb_lhs_contains_rhs( const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr );
    bool_t ego_aabb_overlap( const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr );

    bool_t ego_aabb_copy( ego_aabb_t * pdst, const ego_aabb_t * psrc );
    bool_t ego_aabb_from_oct_bb( ego_aabb_t * dst, const struct s_oct_bb * src );

    bool_t ego_aabb_validate( ego_aabb_t * rhs );
    bool_t ego_aabb_test( const ego_aabb_t * rhs );

//--------------------------------------------------------------------------------------------
    struct s_egolib_frustum
    {
        // basic frustum data
        frustum_base_t data;

        // data for intersection optimization
        fvec3_t   origin;
        sphere_t  sphere;
        cone_t    cone;
    };

/// Call this every time the camera moves to update the frustum
    egolib_rv egolib_frustum_calculate( egolib_frustum_t * pfrust, const float proj[], const float modl[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Call this every time the camera moves to update the frustum
    geometry_rv egolib_frustum_intersects_ego_aabb( const egolib_frustum_t * pfrust, const ego_aabb_t * paabb );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_frustum_h

