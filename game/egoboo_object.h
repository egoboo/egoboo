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
#include "egoboo_state_machine.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some basic data that all Egoboo objects should have

/// The possible states of an obj_data_t object. The object is essentially
/// a state machine in the same way that the "egoboo process" is, so they use analagous
/// states
enum e_ego_object_state
{
    ego_object_invalid        = ego_state_invalid,
    ego_object_constructing   = ego_state_begin,     ///< The object has been allocated and had it's critical variables filled with safe values
    ego_object_initializing   = ego_state_entering,  ///< The object is being initialized/re-initialized
    ego_object_active         = ego_state_running,   ///< The object is fully activated
    ego_object_deinitializing = ego_state_leaving,   ///< The object is being de-initialized
    ego_object_destructing    = ego_state_finish,    ///< The object is being destructed

    // the states that are specific to objects
    ego_object_waiting,                         ///< The object has been fully destructed and is awaiting final "deletion"
    ego_object_terminated                       ///< The object is fully "deleted" and should now be on the free-store
};
typedef enum e_ego_object_state ego_object_state_t;

//--------------------------------------------------------------------------------------------

/// The data that is "inherited" by every Egoboo object.
struct s_ego_object_base
{
    // basic object definitions
    STRING             _name;     ///< what is its "_name"
    size_t             index;     ///< what is the index position in the object list?
    ego_object_state_t state;     ///< what state is it in?
    Uint32             guid;      ///< a globally unique identifier

    // "process" control control
    bool_t             allocated;   ///< Does it exist?
    bool_t             on;          ///< Can it be accessed?
    bool_t             turn_me_on;  ///< Has someone requested that the object be turned on?
    bool_t             kill_me;     ///< Has someone requested that the object be destroyed?
    bool_t             spawning;    ///< is the object in the midst of being created?

    bool_t             in_free_list; ///< the object is currently in the free list
    bool_t             in_used_list; ///< the object is currently in the used list

    // things related to the updating of objects
    size_t         update_count;  ///< How many updates have been made to this object?
    size_t         frame_count;   ///< How many frames have been rendered?

    unsigned       update_guid;   ///< a value that lets you know if an object bookmark is in synch with the object list
};

typedef struct s_ego_object_base obj_data_t;

obj_data_t * ego_object_ctor( obj_data_t * pbase );
obj_data_t * ego_object_dtor( obj_data_t * pbase );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Mark a obj_data_t object as being allocated
#define POBJ_ALLOCATE( PDATA, INDEX ) \
    if( NULL != PDATA ) \
    { \
        (PDATA)->obj_base.allocated  = btrue;  \
        (PDATA)->obj_base.on         = bfalse; \
        (PDATA)->obj_base.turn_me_on = bfalse; \
        (PDATA)->obj_base.kill_me    = bfalse; \
        (PDATA)->obj_base.spawning   = bfalse; \
        (PDATA)->obj_base.index      = INDEX;  \
        (PDATA)->obj_base.state      = ego_object_constructing; \
        (PDATA)->obj_base.guid       = ego_object_guid++; \
    }

/// Turn on an obj_data_t object
#define POBJ_ACTIVATE( PDATA, NAME ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated && !(PDATA)->obj_base.kill_me && ego_object_invalid != (PDATA)->obj_base.state ) \
    { \
        strncpy( (PDATA)->obj_base._name, NAME, SDL_arraysize((PDATA)->obj_base._name) ); \
        (PDATA)->obj_base.state  = ego_object_active; \
    }

/// Begin turning off an obj_data_t object
#define POBJ_REQUEST_TERMINATE( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated && ego_object_invalid != (PDATA)->obj_base.state ) \
    { \
        if( ego_object_terminated != (PDATA)->obj_base.state ) \
        { \
            (PDATA)->obj_base.kill_me = btrue; \
        } \
        (PDATA)->obj_base.on = bfalse; \
    }

/// Completely turn off an obj_data_t object and mark it as no longer allocated
#define POBJ_TERMINATE( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    { \
        (PDATA)->obj_base.allocated = bfalse; \
        (PDATA)->obj_base.on        = bfalse; \
        (PDATA)->obj_base.state     = ego_object_terminated; \
    }

#define POBJ_BEGIN_SPAWN( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    {\
        if( !(PDATA)->obj_base.spawning )\
        {\
            (PDATA)->obj_base.spawning = btrue;\
            ego_object_spawn_depth++;\
        }\
    }\
     
#define POBJ_END_SPAWN( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    {\
        if( (PDATA)->obj_base.spawning )\
        {\
            (PDATA)->obj_base.spawning = bfalse;\
            ego_object_spawn_depth--;\
        }\
    }\
     
/// Is the object flagged as requesting termination?
#define FLAG_ALLOCATED_PBASE( PBASE ) ( ( (PBASE)->allocated ) && (ego_object_invalid != (PBASE)->state) )
/// Is the object allocated?
#define ALLOCATED_PBASE( PBASE )       FLAG_ALLOCATED_PBASE(PBASE)

/// Is the object flagged as requesting termination?
#define FLAG_ON_PBASE( PBASE )  ( (PBASE)->on )
/// Is the object on?
#define ON_PBASE( PBASE )       ( FLAG_ON_PBASE(PBASE) && (ego_object_invalid != (PBASE)->state) )

/// Is the object flagged as kill_me?
#define FLAG_REQ_TERMINATION_PBASE( PBASE ) ( (PBASE)->kill_me )
/// Is the object kill_me?
#define REQ_TERMINATION_PBASE( PBASE )      ( FLAG_REQ_TERMINATION_PBASE(PBASE) && (ego_object_invalid != (PBASE)->state)  )

/// Has the object been created yet?
#define STATE_CONSTRUCTING_PBASE( PBASE ) ( ego_object_constructing == (PBASE)->state )
/// Has the object been created yet?
#define CONSTRUCTING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_CONSTRUCTING_PBASE(PBASE) )

/// Is the object in the initializing state?
#define STATE_INITIALIZING_PBASE( PBASE ) ( ego_object_initializing == (PBASE)->state )
/// Is the object being initialized right now?
#define INITIALIZING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_INITIALIZING_PBASE(PBASE) )

/// Is the object in the active state?
#define STATE_ACTIVE_PBASE( PBASE ) ( ego_object_active == (PBASE)->state )
/// Is the object active?
#define ACTIVE_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_ACTIVE_PBASE(PBASE) )

/// Is the object in the deinitializing state?
#define STATE_DEINITIALIZING_PBASE( PBASE ) ( ego_object_deinitializing == (PBASE)->state )
/// Is the object being deinitialized right now?
#define DEINITIALIZING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_DEINITIALIZING_PBASE(PBASE) )

/// Is the object in the destructing state?
#define STATE_DESTRUCTING_PBASE( PBASE ) ( ego_object_destructing == (PBASE)->state )
/// Is the object being deinitialized right now?
#define DESTRUCTING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_DESTRUCTING_PBASE(PBASE) )

/// Is the object "waiting to die" state?
#define STATE_WAITING_PBASE( PBASE ) ( ego_object_waiting == (PBASE)->state )
/// Is the object "waiting to die"?
#define WAITING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_WAITING_PBASE(PBASE) )

/// Has the object in the terminated state?
#define STATE_TERMINATED_PBASE( PBASE ) ( ego_object_terminated == (PBASE)->state )
/// Has the object been marked as terminated?
#define TERMINATED_PBASE( PBASE )       STATE_TERMINATED_PBASE(PBASE)

/// Grab a pointer to the obj_data_t of an object that "inherits" this data
#define POBJ_GET_PBASE( POBJ )   ( &((POBJ)->obj_base) )

/// Grab the index value of object that "inherits" from obj_data_t
#define GET_INDEX_POBJ( POBJ, FAIL_VALUE )  ( !ALLOCATED_PBASE( POBJ_GET_PBASE( POBJ ) ) ? FAIL_VALUE : (POBJ)->obj_base.index )
#define GET_REF_POBJ( POBJ, FAIL_VALUE )    ((REF_T)GET_INDEX_POBJ( POBJ, FAIL_VALUE ))

/// Grab the state of object that "inherits" from obj_data_t
#define GET_STATE_POBJ( POBJ )  ( !ALLOCATED_PBASE( POBJ_GET_PBASE( POBJ ) ) ? ego_object_invalid : (POBJ)->obj_base.index )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A variable to hold the object guid counter
extern Uint32 ego_object_guid;

extern Uint32 ego_object_spawn_depth;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define egoboo_object_h