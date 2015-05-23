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
#include "egolib/bsp.h"

namespace Ego {
    /**
     * @brief
     *    Data related to the totality of entities.
     */
    struct Entities
    {
        /**
         * @brief
         *    The number of entities currently being spawned.
         */
        static Uint32 spawnDepth;
        /**
         * @brief
         *    The next free entity GUID.
         */
        static Ego::GUID nextGUID;

    };
};

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
            /// The entity is in its invalid state i.e. its memory is not initialized.
            Invalid = 0,
            /// The entity is being constructed.
            Constructing,
            /// The entity is being initialized/re-initialized.
            Initializing,
            /// The entity is active.
            /// The successor to this state is the "deinitializing" state.
            Active,
            /// The "deinitializing" state: The entity is being de-initialized.
            /// Successor to this state is the "destructing" state.
            DeInitializing,
            /// The entity is being destructed.
            Destructing,

            /// The entity was destructed is awaiting "deletion".
            Waiting,
            /// The entity was "deleted". It should be moved to the "free" list and shall have its state set to "invalid".
            Terminated,
        };
        // basic object definitions
        STRING             _name;      ///< what is its "_name"
        size_t             index;      ///< what is the index position in the object list?
        Entity::State      state;      ///< what state is it in?
        Ego::GUID          guid;       ///< a globally unique identifier

        // "process" control control
    protected:
        bool             allocated;    ///< Does it exist?
    public:
        bool             on;           ///< Can it be accessed?
        bool             turn_me_on;   ///< Has someone requested that the object be turned on?
        bool             kill_me;      ///< Has someone requested that the object be destroyed?
        bool             spawning;     ///< is the object in the midst of being created?

        bool             in_free_list; ///< the object is currently in the free list
        bool             in_used_list; ///< the object is currently in the used list

        /// Things related to the updating of objects.
        size_t         update_count;   ///< How many updates have been made to this object?
        size_t         frame_count;    ///< How many frames have been rendered?

        Ego::GUID      update_guid;       ///< a value that lets you know if an object bookmark is in synch with the object list

        /// The BSP leaf for this object.
        /// Moved to here so that is is not destroyed in the destructor of the inherited object.
        BSP_leaf_t     bsp_leaf;

        Ego::Entity *ctor(void *child_data, bsp_type_t child_type, size_t child_index);
        Ego::Entity *dtor();

        /// @brief Is this entity is marked as "allocated"?
        /// @return @a true if this entity is marked as "allocated", @a false otherwise
        bool isAllocated() const
        {
            return allocated;
        }
        /// @brief Mark this entity as "allocated".
        void setAllocated()
        {
            allocated = true;
        }
        /// @brief Allocate and enter state "constructing".
        /// @pre State must be "invalid" or "terminated".
        void allocate(size_t INDEX)
        {
            if (state != State::Invalid && state != State::Terminated)
            {
                log_warning("%s:%d: entity state is neither `invalid` nor `terminated`\n",__FILE__,__LINE__);
            }
            if (allocated)
            {
                log_warning("%s:%d: trying allocate an already allocated entity\n", __FILE__, __LINE__);
            }
            allocated = true;
            on = false;
            turn_me_on = false;
            kill_me = false;
            spawning = false;
            index = INDEX;
            state = State::Constructing;
            guid = Ego::Entities::nextGUID++;
        }
        // Move to state "terminated" and mark as "not allocated".
        void terminate()
        {
            if (!isAllocated())
            {
                log_warning("%s:%d: entity is not allocated\n",__FILE__,__LINE__);
                return;
            }
            allocated = false;
            on = false;
            state = State::Terminated;
        }
    };
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Begin turning off an entity.
/// @todo Make this a function.
#define POBJ_REQUEST_TERMINATE( PDATA ) \
    if( NULL != (PDATA) && (PDATA)->obj_base.isAllocated() && Ego::Entity::State::Invalid != (PDATA)->obj_base.state ) \
    { \
        if( Ego::Entity::State::Terminated != (PDATA)->obj_base.state ) \
        { \
            (PDATA)->obj_base.kill_me = true; \
        } \
        (PDATA)->obj_base.on = false; \
    }

#define POBJ_END_SPAWN( PDATA ) \
    if( NULL != (PDATA) && (PDATA)->obj_base.isAllocated()) \
    {\
        if( (PDATA)->obj_base.spawning )\
        {\
            (PDATA)->obj_base.spawning = false;\
            Ego::Entities::spawnDepth--;\
        }\
    }

/// Is the object flagged as requesting termination?
#define FLAG_ALLOCATED_PBASE( PBASE ) ( ( (PBASE)->isAllocated() ) && (Ego::Entity::State::Invalid != (PBASE)->state) )
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

/// @todo Remove this.
template <typename TYPE,typename LISTTYPE>
struct _StateMachine
{
    Ego::Entity obj_base; ///< The "inheritance" from Ego::Entity.

    _StateMachine() :
        obj_base()
    {
    }

    virtual ~_StateMachine()
    {
    }

    static TYPE *config_ctor(TYPE *self)
    {
        if (!self) return nullptr;

        // Get a pointer to the parent.
        Ego::Entity *parent = POBJ_GET_PBASE(self);

        // If we aren't in the correct state, abort.
        if (!STATE_CONSTRUCTING_PBASE(parent)) return self;

        return self->config_do_ctor();
    }

    static TYPE *config_active(TYPE *self)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!parent->isAllocated()) return nullptr;

        if (!STATE_ACTIVE_PBASE(parent)) return self;

        POBJ_END_SPAWN(self);

        return self->config_do_active();
    }

    static void config_deinit(TYPE& self)
    {
        Ego::Entity *parent = POBJ_GET_PBASE(&self);

        if (!STATE_DEINITIALIZING_PBASE(parent)) return;

        POBJ_END_SPAWN(&self);

        self.config_do_deinit();
    }

    static void config_dtor(TYPE& self)
    {
        Ego::Entity *parent = POBJ_GET_PBASE(&self);

        if (!STATE_DESTRUCTING_PBASE(parent));

        POBJ_END_SPAWN(&self);

        self.config_do_dtor();
    }

    static TYPE *config_initialize(TYPE *self, size_t max_iterations)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!parent->isAllocated()) return nullptr;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Initializing)
        {
            // ... deconstruct it and start over.
            TYPE *tmp = TYPE::config_deconstruct(self, max_iterations);
            if (tmp != self) return nullptr;
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::State::Initializing && iterations < max_iterations)
        {
            TYPE *tmp = TYPE::run_config(self);
            if (tmp != self) return nullptr;
            iterations++;
        }

        return self;
    }

    static TYPE *config_construct(TYPE *self, size_t max_iterations)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!parent->isAllocated()) return nullptr;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Constructing)
        {
            // ... destruct it and start over.
            TYPE *tmp = TYPE::config_deconstruct(self, max_iterations);
            if (tmp != self) return nullptr;
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::Constructing && iterations < max_iterations)
        {
            TYPE *tmp = TYPE::run_config(self);
            if (tmp != self) return nullptr;
            iterations++;
        }

        return self;
    }

    static TYPE *config_init(TYPE *self)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!STATE_INITIALIZING_PBASE(parent)) return self;

        self = self->config_do_init();
        if (!self) return nullptr;

        if (0 == LISTTYPE::get().getLockCount())
        {
            parent->on = true;
        }
        else
        {
            LISTTYPE::get().add_activation(self->obj_base.index);
        }

        parent->state = Ego::Entity::State::Active;

        return self;
    }

    static TYPE *config_deinitialize(TYPE *self, size_t max_iterations)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!parent->isAllocated()) return nullptr;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::DeInitializing)
        {
            // ... do nothing.
            return self;
        }
        else if (parent->state < Ego::Entity::State::DeInitializing)
        {
            parent->state = Ego::Entity::State::DeInitializing;
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::State::DeInitializing && iterations < max_iterations)
        {
            TYPE *tmp = TYPE::run_config(self);
            if (tmp != self) return nullptr;
            iterations++;
        }

        return self;
    }

    static TYPE *config_activate(TYPE *self, size_t max_iterations)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!parent->isAllocated()) return nullptr;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Active)
        {
            // ... deconstruct it and start over.
            TYPE *tmp = TYPE::config_deconstruct(self, max_iterations);
            if (tmp != self) return nullptr;
        }

        size_t iterations = 0;
        while (parent->state < Ego::Entity::State::Active && iterations < max_iterations)
        {
            TYPE *tmp = TYPE::run_config(self);
            if (tmp != self) return nullptr;
            iterations++;
        }

        if (parent->state == Ego::Entity::State::Active)
        {
            LISTTYPE::get().push_used(self->obj_base.index);
        }

        return self;
    }

    static TYPE *config_deconstruct(TYPE *self, size_t max_iterations)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!parent->isAllocated()) return nullptr;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Destructing)
        {
            // ... do nothing.
            return self;
        }
        else if (parent->state < Ego::Entity::State::DeInitializing)
        {
            // Make sure that you deinitialize before destructing.
            parent->state = Ego::Entity::State::DeInitializing;
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::State::Destructing && iterations < max_iterations)
        {
            TYPE *tmp = TYPE::run_config(self);
            if (tmp != self) return nullptr;
            iterations++;
        }
        if (parent->state < Ego::Entity::Terminated)
        {
            if (parent->state < Ego::Entity::Destructing)
            {
                log_warning("%s:%d: entity is not destructed\n", __FILE__, __LINE__);
            }
            log_warning("%s:%d: entity is not destructed\n", __FILE__, __LINE__);
        }
        return self;
    }

    static TYPE *run_config(TYPE *self)
    {
        if (!self) return nullptr;

        Ego::Entity *parent = POBJ_GET_PBASE(self);
        if (!parent->isAllocated()) return nullptr;

        // Set the object to deinitialize if it is not "dangerous" and if was requested.
        if (parent->kill_me)
        {
            if (parent->state > Ego::Entity::State::Constructing && parent->state < Ego::Entity::State::DeInitializing)
            {
                parent->state = Ego::Entity::State::DeInitializing;
            }

            parent->kill_me = false;
        }

        switch (parent->state)
        {
        default:
        case Ego::Entity::State::Invalid:
            self = nullptr;
            break;

        case Ego::Entity::State::Constructing:
            self = TYPE::config_ctor(self);
            break;

        case Ego::Entity::State::Initializing:
            self = TYPE::config_init(self);
            break;

        case Ego::Entity::State::Active:
            self = TYPE::config_active(self);
            break;

        case Ego::Entity::State::DeInitializing:
            TYPE::config_deinit(*self);
            break;

        case Ego::Entity::State::Destructing:
            TYPE::config_dtor(*self);
            break;

        case Ego::Entity::State::Waiting:
        case Ego::Entity::State::Terminated:
            /* Do nothing. */
            break;
        }

        if (!self)
        {
            parent->update_guid = EGO_GUID_INVALID;
        }
        else if (Ego::Entity::State::Active == parent->state)
        {
            parent->update_guid = LISTTYPE::get().getUpdateGUID();
        }

        return self;
    }


};