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

/// @file egoboo_object.h
/// @details Definitions of data that all Egoboo objects should "inherit"

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some basic data that all Egoboo objects should have

/// The possible states of an ego_object_base_t object
enum e_ego_object_state
{
    ego_object_invalid = 0,
    ego_object_pre_active,               ///< in the process of being activated
    ego_object_active,                   ///< fully activated
    ego_object_waiting,                  ///< waiting to be terminated
    ego_object_terminated                ///< fully terminated
};

//--------------------------------------------------------------------------------------------
/// The data that is "inherited" by every Egoboo object.
struct s_ego_object_base
{
    // basic object definitions
    STRING         _name;     ///< what is its "_name"
    size_t         index;     ///< what is the index position in the object list?
    bool_t         allocated; ///< Does it exist?
    int            state;     ///< what state is it in?
    Uint32         guid;      ///< a globally unique identifier

    // things related to spawning/despawning
    size_t         update_count;    ///< How many updates have been made to this object?
    size_t         frame_count;     ///< How many frames have been rendered?

};

typedef struct s_ego_object_base ego_object_base_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// various modes in which an object can be spawned
enum
{
    EGO_OBJECT_DO_ALLOCATE,
    EGO_OBJECT_DO_ACTIVATE,
    EGO_OBJECT_DO_NOTHING
};

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

/// Is the object in the allocated state?
#define STATE_ALLOCATED_PBASE( PBASE ) ( ( (PBASE)->allocated ) && (ego_object_invalid != (PBASE)->state) )
/// Is the object allocated?
#define ALLOCATED_PBASE( PBASE )       ( (NULL != (PBASE)) && STATE_ALLOCATED_PBASE(PBASE) )

/// Is the object "on" state?
#define STATE_ACTIVE_PBASE( PBASE ) ( ego_object_active == (PBASE)->state )
/// Is the object "on"?
#define ACTIVE_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_ACTIVE_PBASE(PBASE) )

/// Is the object "waiting to die" state?
#define STATE_WAITING_PBASE( PBASE ) ( ego_object_waiting == (PBASE)->state )
/// Is the object "waiting to die"?
#define WAITING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_WAITING_PBASE(PBASE) )

/// Has the object in the terminated state?
#define STATE_TERMINATED_PBASE( PBASE ) ( ego_object_terminated == (PBASE)->state )
/// Has the object been marked as terminated?
#define TERMINATED_PBASE( PBASE )       ( (NULL != (PBASE)) && STATE_TERMINATED_PBASE(PBASE) )

/// Grab a pointer to the ego_object_base_t of an object that "inherits" this data
#define POBJ_GET_PBASE( POBJ )   ( (NULL == (POBJ)) ? NULL : &((POBJ)->obj_base) )

/// Grab the index value of object that "inherits" from ego_object_base_t
#define GET_INDEX_POBJ( POBJ, FAIL_VALUE )  ( (NULL == (POBJ) || !ALLOCATED_PBASE( POBJ_GET_PBASE( (POBJ) ) ) ) ? FAIL_VALUE : (POBJ)->obj_base.index )
#define GET_REF_POBJ( POBJ, FAIL_VALUE )    ((REF_T)GET_INDEX_POBJ( POBJ, FAIL_VALUE ))

/// Grab the state of object that "inherits" from ego_object_base_t
#define GET_STATE_POBJ( POBJ )  ( (NULL == (POBJ) || !ALLOCATED_PBASE( POBJ_GET_PBASE( (POBJ) ) ) ) ? ego_object_invalid : (POBJ)->obj_base.index )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// A variable to hold the object guid counter
extern Uint32 ego_object_guid;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define egoboo_object_h