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
        STRING     _name;      ///< what is its "_name"
        size_t     index;      ///< what is the index position in the object list?
        State      state;      ///< what state is it in?
        GUID       guid;       ///< a globally unique identifier

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

        GUID      update_guid;       ///< a value that lets you know if an object bookmark is in synch with the object list

        /// The BSP leaf for this object.
        /// Moved to here so that is is not destroyed in the destructor of the inherited object.
        BSP_leaf_t     bsp_leaf;

		Entity(void *child_data, bsp_type_t child_type, size_t child_index);
        void reset();
        Entity *dtor();

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
            guid = Entities::nextGUID++;
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
        /// Is the object flagged as requesting termination?
        inline bool FLAG_ALLOCATED_PBASE() const {
            return isAllocated() && (State::Invalid != state);
        }

        /// Is the object allocated?
        inline bool ALLOCATED_PBASE() const {
            return FLAG_ALLOCATED_PBASE();
        }

        /// Is the object flagged as requesting termination?
        inline bool FLAG_ON_PBASE() const {
            return on;
        }

        /// Is the object on?
        inline bool ON_PBASE() const {
            return FLAG_ON_PBASE() && (State::Invalid != state);
        }

        /// Is the object flagged as kill_me?
        inline bool FLAG_REQ_TERMINATION_PBASE() const {
            return kill_me;
        }

        /// Is the object kill_me?
        inline bool REQ_TERMINATION_PBASE() const {
            return FLAG_REQ_TERMINATION_PBASE() && (State::Invalid != state);
        }

        /// Has the object been created yet?
        inline bool STATE_CONSTRUCTING_PBASE() const {
            return State::Constructing == state;
        }

        /// Has the object been created yet?
        inline bool CONSTRUCTING_PBASE() const {
            return ALLOCATED_PBASE() && STATE_CONSTRUCTING_PBASE();
        }

        /// Is the object in the initializing state?
        inline bool STATE_INITIALIZING_PBASE() const {
            return State::Initializing == state;
        }

        /// Is the object being initialized right now?
        inline bool INITIALIZING_PBASE() const {
            return ALLOCATED_PBASE() && STATE_INITIALIZING_PBASE();
        }

        /// Is the object in the active state?
        inline bool STATE_ACTIVE_PBASE() const {
            return State::Active == state;
        }

        /// Is the object active?
        inline bool ACTIVE_PBASE() const {
            return ALLOCATED_PBASE() && STATE_ACTIVE_PBASE();
        }

        /// Is the object in the deinitializing state?
        inline bool STATE_DEINITIALIZING_PBASE() const {
            return State::DeInitializing == state;
        }

        /// Is the object being deinitialized right now?
        inline bool DEINITIALIZING_PBASE() const {
            return ALLOCATED_PBASE() && STATE_DEINITIALIZING_PBASE();
        }

        /// Is the object in the destructing state?
        inline bool STATE_DESTRUCTING_PBASE() const {
            return State::Destructing == state;
        }

        /// Is the object being deinitialized right now?
        inline bool DESTRUCTING_PBASE() const {
            return ALLOCATED_PBASE() && STATE_DESTRUCTING_PBASE();
        }

        /// Is the object "waiting to die" state?
        inline bool STATE_WAITING_PBASE() const {
            return State::Waiting == state;
        }

        /// Is the object "waiting to die"?
        inline bool WAITING_PBASE() const {
            return ALLOCATED_PBASE() && STATE_WAITING_PBASE();
        }

        /// Has the object in the terminated state?
        inline bool STATE_TERMINATED_PBASE() const {
            return State::Terminated == state;
        }

        /// Has the object been marked as terminated?
        inline bool TERMINATED_PBASE() const {
            return STATE_TERMINATED_PBASE();
        }

    };
} // namespace Ego

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


/// Grab a pointer to the Ego::Entity of an object that "inherits" this data
#define POBJ_GET_PBASE( POBJ )   ( &((POBJ)->obj_base) )

/// Grab a pointer to the BSP_leaf_t of an object that "inherits" this data
#define POBJ_GET_PLEAF( POBJ )   LAMBDA( NULL == (POBJ), NULL, &((POBJ)->obj_base.bsp_leaf) )
/// Grab a pointer to the BSP_leaf_t of an object that "inherits" this data
#define OBJ_GET_LEAF( POBJ )   ( (OBJ).obj_base.bsp_leaf )

/// Grab the index value of object that "inherits" from Ego::Entity
#define GET_INDEX_POBJ( POBJ, FAIL_VALUE )  LAMBDA(!POBJ_GET_PBASE(POBJ)->ALLOCATED_PBASE(), FAIL_VALUE, (POBJ)->obj_base.index)
#define GET_REF_POBJ( POBJ, FAIL_VALUE )    ((REF_T)GET_INDEX_POBJ( POBJ, FAIL_VALUE ))

/// Grab the state of object that "inherits" from Ego::Entity
#define GET_STATE_POBJ( POBJ )  LAMBDA( !ALLOCATED_PBASE( POBJ_GET_PBASE( POBJ ) ), ego_object_invalid, (POBJ)->obj_base.index )

//--------------------------------------------------------------------------------------------

/// @todo Remove this.
template <typename TYPE,typename REFTYPE, typename LISTTYPE>
struct _StateMachine
{
    Ego::Entity obj_base; ///< The "inheritance" from Ego::Entity.

	_StateMachine(bsp_type_t child_type, size_t child_index) :
        obj_base(this, child_type, child_index) {
    }

    virtual ~_StateMachine() {
    }

	void reset() {
		obj_base.reset();
	}

    // state machine function
    virtual TYPE *config_do_ctor() = 0;
    // state machine function
    virtual TYPE *config_do_init() = 0;
    // state machine function
    virtual TYPE *config_do_active() = 0;
    // state machine function
    virtual void config_do_deinit() = 0;
    // state machine function
    virtual void config_do_dtor() = 0;

    bool config_ctor()
    {
        // Get a pointer to the parent.
        Ego::Entity *parent = POBJ_GET_PBASE(this);

        // If we aren't in the correct state, abort.
        if (!parent->STATE_CONSTRUCTING_PBASE()) return true;

        return nullptr != this->config_do_ctor();
    }

    bool config_active()
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->isAllocated()) return false;

        if (!parent->STATE_ACTIVE_PBASE()) return true;

        POBJ_END_SPAWN(this);

        return nullptr != this->config_do_active();
    }

    void config_deinit()
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);

        if (!parent->STATE_DEINITIALIZING_PBASE()) return;

        POBJ_END_SPAWN(this);

        config_do_deinit();
    }

    void config_dtor()
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);

		if (!parent->STATE_DESTRUCTING_PBASE()) return;

        POBJ_END_SPAWN(this);

        config_do_dtor();
    }

    bool config_initialize(size_t max_iterations)
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Initializing)
        {
            // ... deconstruct it and start over.
            if (!this->config_deconstruct(max_iterations)) {
                return false;
            }
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::State::Initializing && iterations < max_iterations)
        {
            if (!this->run_config()) {
                return false;
            }
            iterations++;
        }

        return true;
    }

    bool config_construct(size_t max_iterations)
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Constructing)
        {
            // ... destruct it and start over.
            if (!this->config_deconstruct(max_iterations)) {
                return false;
            }
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::Constructing && iterations < max_iterations)
        {
            if (!this->run_config()) {
                return false;
            }
            iterations++;
        }

        return true;
    }

    bool config_init()
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->STATE_INITIALIZING_PBASE()) return true;

        if (!this->config_do_init()) {
            return false;
        }

        if (0 == LISTTYPE::get().getLockCount())
        {
            parent->on = true;
        }
        else
        {
            LISTTYPE::get().add_activation(this->obj_base.index);
        }

        parent->state = Ego::Entity::State::Active;

        return true;
    }

    bool config_deinitialize(size_t max_iterations)
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::DeInitializing)
        {
            // ... do nothing.
            return true;
        }
        else if (parent->state < Ego::Entity::State::DeInitializing)
        {
            parent->state = Ego::Entity::State::DeInitializing;
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::State::DeInitializing && iterations < max_iterations)
        {
            if (!this->run_config()) {
                return false;
            }
            iterations++;
        }

        return true;
    }

    bool config_activate(size_t max_iterations)
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Active)
        {
            // ... deconstruct it and start over.
            if (!this->config_deconstruct(max_iterations)) {
                return false;
            }
        }

        size_t iterations = 0;
        while (parent->state < Ego::Entity::State::Active && iterations < max_iterations)
        {
            if (!this->run_config()) {
                return false;
            }
            iterations++;
        }

        if (parent->state == Ego::Entity::State::Active)
        {
            LISTTYPE::get().push_used(this->obj_base.index);
        }

        return true;
    }

    bool config_deconstruct(size_t max_iterations)
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (parent->state > Ego::Entity::State::Destructing)
        {
            // ... do nothing.
            return true;
        }
        else if (parent->state < Ego::Entity::State::DeInitializing)
        {
            // Make sure that you deinitialize before destructing.
            parent->state = Ego::Entity::State::DeInitializing;
        }

        size_t iterations = 0;
        while (parent->state <= Ego::Entity::State::Destructing && iterations < max_iterations)
        {
            if (!this->run_config()) {
                return false;
            }
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
        return true;
    }

    bool run_config()
    {
        Ego::Entity *parent = POBJ_GET_PBASE(this);
        if (!parent->isAllocated()) return false;

        // Set the object to deinitialize if it is not "dangerous" and if was requested.
        if (parent->kill_me)
        {
            if (parent->state > Ego::Entity::State::Constructing && parent->state < Ego::Entity::State::DeInitializing)
            {
                parent->state = Ego::Entity::State::DeInitializing;
            }

            parent->kill_me = false;
        }

        bool result = true;
        switch (parent->state)
        {
        default:
        case Ego::Entity::State::Invalid:
            result = false;
            break;

        case Ego::Entity::State::Constructing:
            result = this->config_ctor();
            break;

        case Ego::Entity::State::Initializing:
            result = this->config_init();
            break;

        case Ego::Entity::State::Active:
            result = this->config_active();
            break;

        case Ego::Entity::State::DeInitializing:
            this->config_deinit();
            break;

        case Ego::Entity::State::Destructing:
            this->config_dtor();
            break;

        case Ego::Entity::State::Waiting:
        case Ego::Entity::State::Terminated:
            /* Do nothing. */
            break;
        }

        if (!result)
        {
            parent->update_guid = EGO_GUID_INVALID;
        }
        else if (Ego::Entity::State::Active == parent->state)
        {
            parent->update_guid = LISTTYPE::get().getUpdateGUID();
        }

        return result;
    }

};
