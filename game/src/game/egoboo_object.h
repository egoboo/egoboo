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

/// @file    game/egoboo_object.h
/// @details Definitions of data that all Egoboo objects should "inherit"

#pragma once

#include "game/egoboo_typedef.h"

namespace Ego
{
	/// Entities are mutable elements of the game. Any mutable element of the game
	/// (including but not restricted to characters, particles and enchantments) are
	/// entities. This class encapsulates fields and methods which are inherited by
	/// every entity.
	struct Entity
	{
		/// The possible states of an entity.
		/// An entity is essentially a state machine in the same way that the "Egoboo process" is,
		/// so they use analagous states.
		enum State
		{
			Invalid = ego_state_invalid,
			Constructing = ego_state_begin,     ///< The object has been allocated and had it's critical variables filled with safe values
			Initializing = ego_state_entering,  ///< The object is being initialized/re-initialized
			Active = ego_state_running,         ///< The object is fully activated
			DeInitializing = ego_state_leaving, ///< The object is being de-initialized
			Destructing = ego_state_finish,     ///< The object is being destructed

			// the states that are specific to objects
			Waiting,                            ///< The object has been fully destructed and is awaiting final "deletion"
			Terminated,                         ///< The object is fully "deleted" and should now be on the free-store
		};
		// basic object definitions
		STRING             _name;      ///< what is its "_name"
		size_t             index;      ///< what is the index position in the object list?
		Entity::State      state;      ///< what state is it in?
		Uint32             guid;       ///< a globally unique identifier

		// "process" control control
		bool             allocated;    ///< Does it exist?
		bool             on;           ///< Can it be accessed?
		bool             turn_me_on;   ///< Has someone requested that the object be turned on?
		bool             kill_me;      ///< Has someone requested that the object be destroyed?
		bool             spawning;     ///< is the object in the midst of being created?

		bool             in_free_list; ///< the object is currently in the free list
		bool             in_used_list; ///< the object is currently in the used list

		/// Things related to the updating of objects.
		size_t         update_count;   ///< How many updates have been made to this object?
		size_t         frame_count;    ///< How many frames have been rendered?

		unsigned       update_guid;    ///< a value that lets you know if an object bookmark is in synch with the object list

		/// The BSP leaf for this object.
		/// Moved to here so that is is not destroyed in the destructor of the inherited object.
		BSP_leaf_t     bsp_leaf;
	};
};

Ego::Entity *ego_object_ctor(Ego::Entity *pbase, void *child_data, int child_type, size_t child_index);
Ego::Entity *ego_object_dtor(Ego::Entity *pbase);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Mark an entity as being allocated.
/// @todo Make this a function.
#define POBJ_ALLOCATE( PDATA, INDEX ) \
    if( NULL != PDATA ) \
	    { \
        (PDATA)->obj_base.allocated  = true;  \
        (PDATA)->obj_base.on         = false; \
        (PDATA)->obj_base.turn_me_on = false; \
        (PDATA)->obj_base.kill_me    = false; \
        (PDATA)->obj_base.spawning   = false; \
        (PDATA)->obj_base.index      = INDEX;  \
        (PDATA)->obj_base.state      = Ego::Entity::State::Constructing; \
        (PDATA)->obj_base.guid       = ego_object_guid++; \
	    }

/// Turn on an entity.
/// @todo Make this a function.
#define POBJ_ACTIVATE( PDATA, NAME ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated && !(PDATA)->obj_base.kill_me && Ego::Entity::State::Invalid != (PDATA)->obj_base.state ) \
	    { \
        strncpy( (PDATA)->obj_base._name, NAME, SDL_arraysize((PDATA)->obj_base._name) ); \
        (PDATA)->obj_base.state  = Ego::Entity::State::Active; \
	    }

/// Begin turning off an entity.
/// @todo Make this a function.
#define POBJ_REQUEST_TERMINATE( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated && Ego::Entity::State::Invalid != (PDATA)->obj_base.state ) \
	    { \
        if( Ego::Entity::State::Terminated != (PDATA)->obj_base.state ) \
		        { \
            (PDATA)->obj_base.kill_me = true; \
		        } \
        (PDATA)->obj_base.on = false; \
	    }

/// Completely turn off an entity and mark it as no longer allocated.
/// @todo Make this a function.
#define POBJ_TERMINATE( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
	    { \
        (PDATA)->obj_base.allocated = false; \
        (PDATA)->obj_base.on        = false; \
        (PDATA)->obj_base.state     = Ego::Entity::State::Terminated; \
    }

#define POBJ_BEGIN_SPAWN( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    {\
        if( !(PDATA)->obj_base.spawning )\
        {\
            (PDATA)->obj_base.spawning = true;\
            ego_object_spawn_depth++;\
        }\
    }\

#define POBJ_END_SPAWN( PDATA ) \
    if( NULL != PDATA && (PDATA)->obj_base.allocated ) \
    {\
        if( (PDATA)->obj_base.spawning )\
        {\
            (PDATA)->obj_base.spawning = false;\
            ego_object_spawn_depth--;\
        }\
    }\

/// Is the object flagged as requesting termination?
#define FLAG_ALLOCATED_PBASE( PBASE ) ( ( (PBASE)->allocated ) && (Ego::Entity::State::Invalid != (PBASE)->state) )
/// Is the object allocated?
#define ALLOCATED_PBASE( PBASE )       FLAG_ALLOCATED_PBASE(PBASE)

/// Is the object flagged as requesting termination?
#define FLAG_ON_PBASE( PBASE )  ( (PBASE)->on )
/// Is the object on?
#define ON_PBASE( PBASE )       ( FLAG_ON_PBASE(PBASE) && (Ego::Entity::State::Invalid != (PBASE)->state) )

/// Is the object flagged as kill_me?
#define FLAG_REQ_TERMINATION_PBASE( PBASE ) ( (PBASE)->kill_me )
/// Is the object kill_me?
#define REQ_TERMINATION_PBASE( PBASE )      ( FLAG_REQ_TERMINATION_PBASE(PBASE) && (Ego::Entity::State::Invalid != (PBASE)->state)  )

/// Has the object been created yet?
#define STATE_CONSTRUCTING_PBASE( PBASE ) ( Ego::Entity::State::Constructing == (PBASE)->state )
/// Has the object been created yet?
#define CONSTRUCTING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_CONSTRUCTING_PBASE(PBASE) )

/// Is the object in the initializing state?
#define STATE_INITIALIZING_PBASE( PBASE ) ( Ego::Entity::State::Initializing == (PBASE)->state )
/// Is the object being initialized right now?
#define INITIALIZING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_INITIALIZING_PBASE(PBASE) )

/// Is the object in the active state?
#define STATE_ACTIVE_PBASE( PBASE ) ( Ego::Entity::State::Active == (PBASE)->state )
/// Is the object active?
#define ACTIVE_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_ACTIVE_PBASE(PBASE) )

/// Is the object in the deinitializing state?
#define STATE_DEINITIALIZING_PBASE( PBASE ) ( Ego::Entity::State::DeInitializing == (PBASE)->state )
/// Is the object being deinitialized right now?
#define DEINITIALIZING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_DEINITIALIZING_PBASE(PBASE) )

/// Is the object in the destructing state?
#define STATE_DESTRUCTING_PBASE( PBASE ) ( Ego::Entity::State::Destructing == (PBASE)->state )
/// Is the object being deinitialized right now?
#define DESTRUCTING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_DESTRUCTING_PBASE(PBASE) )

/// Is the object "waiting to die" state?
#define STATE_WAITING_PBASE( PBASE ) ( Ego::Entity::State::Waiting == (PBASE)->state )
/// Is the object "waiting to die"?
#define WAITING_PBASE( PBASE )       ( ALLOCATED_PBASE( PBASE ) && STATE_WAITING_PBASE(PBASE) )

/// Has the object in the terminated state?
#define STATE_TERMINATED_PBASE( PBASE ) ( Ego::Entity::State::Terminated == (PBASE)->state )
/// Has the object been marked as terminated?
#define TERMINATED_PBASE( PBASE )       STATE_TERMINATED_PBASE(PBASE)

/// Grab a pointer to the Ego::Entity of an object that "inherits" this data
#define POBJ_GET_PBASE( POBJ )   ( &((POBJ)->obj_base) )

/// Grab a pointer to the BSP_leaf_t of an object that "inherits" this data
#define POBJ_GET_PLEAF( POBJ )   LAMBDA( NULL == (POBJ), NULL, &((POBJ)->obj_base.bsp_leaf) )
/// Grab a pointer to the BSP_leaf_t of an object that "inherits" this data
#define OBJ_GET_LEAF( POBJ )   ( (OBJ).obj_base.bsp_leaf )

/// Grab the index value of object that "inherits" from Ego::Entity
#define GET_INDEX_POBJ( POBJ, FAIL_VALUE )  LAMBDA( !ALLOCATED_PBASE( POBJ_GET_PBASE( POBJ ) ), FAIL_VALUE, (POBJ)->obj_base.index )
#define GET_REF_POBJ( POBJ, FAIL_VALUE )    ((REF_T)GET_INDEX_POBJ( POBJ, FAIL_VALUE ))

/// Grab the state of object that "inherits" from Ego::Entity
#define GET_STATE_POBJ( POBJ )  LAMBDA( !ALLOCATED_PBASE( POBJ_GET_PBASE( POBJ ) ), ego_object_invalid, (POBJ)->obj_base.index )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A variable to hold the object guid counter.
extern Uint32 ego_object_guid;

extern Uint32 ego_object_spawn_depth;
