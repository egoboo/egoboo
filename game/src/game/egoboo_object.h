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
        std::string _name;      ///< what is its "_name"
        State       state;      ///< what state is it in?
 
		// "process" control control
    private:
		size_t _index;      ///< what is the index position in the object list?
		GUID _guid;         ///< a globally unique identifier
		bool _allocated;    ///< Does it exist?
		GUID _update_guid;  ///< a value that lets you know if an object bookmark is in synch with the object list
    public:

        bool             on;           ///< Can it be accessed?
        bool             turn_me_on;   ///< Has someone requested that the object be turned on?
        bool             kill_me;      ///< Has someone requested that the object be destroyed?
        bool             spawning;     ///< is the object in the midst of being created?

        bool in_free_list; ///< the object is currently in the free list
        bool in_used_list; ///< the object is currently in the used list

        /// Things related to the updating of objects.
        size_t         update_count;   ///< How many updates have been made to this object?
        size_t         frame_count;    ///< How many frames have been rendered?



        /// The BSP leaf for this object.
        /// Moved to here so that is is not destroyed in the destructor of the inherited object.
        BSP_leaf_t     bsp_leaf;

		Entity(void *child_data, bsp_type_t child_type, size_t child_index);
        void reset();
        Entity *dtor();

		GUID getGUID() const {
			return _guid;
		}

		size_t getIndex() const {
			return _index;
		}

		void setGUID(GUID guid) {
			_guid = guid;
		}

		GUID getUpdateGUID() const {
			return _update_guid;
		}

		void setUpdateGUID(GUID update_guid) {
			_update_guid = update_guid;
		}

        /// @brief Is this entity is marked as "allocated"?
        /// @return @a true if this entity is marked as "allocated", @a false otherwise
        bool isAllocated() const {
            return _allocated;
        }
        /// @brief Mark this entity as "allocated".
        void setAllocated(bool allocated) {
            _allocated = allocated;
        }
        /// @brief Allocate and enter state "constructing".
        /// @pre State must be "invalid" or "terminated".
        void allocate(size_t INDEX)
        {
            if (state != State::Invalid && state != State::Terminated)
            {
                log_warning("%s:%d: entity state is neither `invalid` nor `terminated`\n",__FILE__,__LINE__);
            }
            if (_allocated)
            {
                log_warning("%s:%d: trying allocate an already allocated entity\n", __FILE__, __LINE__);
            }
            _allocated = true;
            on = false;
            turn_me_on = false;
            kill_me = false;
            spawning = false;
            _index = INDEX;
            state = State::Constructing;
            _guid = Entities::nextGUID++;
        }
        // Move to state "terminated" and mark as "not allocated".
        void terminate()
        {
            if (!isAllocated())
            {
                log_warning("%s:%d: entity is not allocated\n",__FILE__,__LINE__);
                return;
            }
            _allocated = false;
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

		/**
		* @brief
		*  Get if an object is in-game.
		* @param ptr
		*  the object
		* @return
		*  @a true if the object is in-game, @a false otherwise
		*/
		bool INGAME_BASE_RAW() const
		{
			return ACTIVE_PBASE()
				&& ON_PBASE();
		}

		/**
		* @brief
		*  Get if an object is defined.
		* @param ptr
		*  the object
		* @return
		*  @a true if the object is defined, @a false otherwise
		*/
		bool DEFINED_BASE_RAW() const
		{
			return ALLOCATED_PBASE()
				&& !TERMINATED_PBASE();
		}


    };
} // namespace Ego

//--------------------------------------------------------------------------------------------

/// @todo Remove this.
template <typename TYPE,typename REFTYPE, typename LISTTYPE>
struct _StateMachine : public Ego::Entity
{
	_StateMachine(bsp_type_t child_type, size_t child_index) :
        Ego::Entity(this, child_type, child_index) {
    }

    virtual ~_StateMachine() {
    }

	/// Grab a pointer to the BSP_leaf_t of an object that "inherits" this data
	BSP_leaf_t *POBJ_GET_PLEAF() {
		return &(bsp_leaf);
	}

	const BSP_leaf_t *POBJ_GET_PLEAF() const {
		return &(bsp_leaf);
	}

	void reset() {
		this->Ego::Entity::reset();
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
        // If we aren't in the correct state, abort.
        if (!this->STATE_CONSTRUCTING_PBASE()) return true;

        return nullptr != this->config_do_ctor();
    }

    bool config_active()
    {
        if (!this->isAllocated()) return false;

        if (!this->STATE_ACTIVE_PBASE()) return true;

		this->POBJ_END_SPAWN();

        return nullptr != this->config_do_active();
    }

    void config_deinit()
    {
        if (!this->STATE_DEINITIALIZING_PBASE()) return;

		this->POBJ_END_SPAWN();

        config_do_deinit();
    }

    void config_dtor()
    {
		if (!this->STATE_DESTRUCTING_PBASE()) return;

		this->POBJ_END_SPAWN();

        config_do_dtor();
    }

    bool config_initialize(size_t max_iterations)
    {
        if (!this->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (this->state > State::Initializing)
        {
            // ... deconstruct it and start over.
            if (!this->config_deconstruct(max_iterations)) {
                return false;
            }
        }

        size_t iterations = 0;
        while (this->state <= State::Initializing && iterations < max_iterations)
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
        if (!this->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (this->state > State::Constructing)
        {
            // ... destruct it and start over.
            if (!this->config_deconstruct(max_iterations)) {
                return false;
            }
        }

        size_t iterations = 0;
        while (this->state <= State::Constructing && iterations < max_iterations)
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
        if (!this->STATE_INITIALIZING_PBASE()) return true;

        if (!this->config_do_init()) {
            return false;
        }

        if (0 == LISTTYPE::get().getLockCount())
        {
            this->on = true;
        }
        else
        {
            LISTTYPE::get().add_activation(this->getIndex());
        }

        this->state = State::Active;

        return true;
    }

    bool config_deinitialize(size_t max_iterations)
    {
        if (!this->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (this->state > State::DeInitializing)
        {
            // ... do nothing.
            return true;
        }
        else if (this->state < State::DeInitializing)
        {
            this->state = State::DeInitializing;
        }

        size_t iterations = 0;
        while (this->state <= State::DeInitializing && iterations < max_iterations)
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
        if (!this->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (this->state > State::Active)
        {
            // ... deconstruct it and start over.
            if (!this->config_deconstruct(max_iterations)) {
                return false;
            }
        }

        size_t iterations = 0;
        while (this->state < State::Active && iterations < max_iterations)
        {
            if (!this->run_config()) {
                return false;
            }
            iterations++;
        }

        if (this->state == State::Active)
        {
            LISTTYPE::get().push_used(this->getIndex());
        }

        return true;
    }

    bool config_deconstruct(size_t max_iterations)
    {
        if (!this->isAllocated()) return false;

        // If the object is already beyond this stage ...
        if (this->state > State::Destructing)
        {
            // ... do nothing.
            return true;
        }
        else if (this->state < State::DeInitializing)
        {
            // Make sure that you deinitialize before destructing.
            this->state = State::DeInitializing;
        }

        size_t iterations = 0;
        while (this->state <= State::Destructing && iterations < max_iterations)
        {
            if (!this->run_config()) {
                return false;
            }
            iterations++;
        }
        if (this->state < State::Terminated)
        {
            if (this->state < State::Destructing)
            {
                log_warning("%s:%d: entity is not destructed\n", __FILE__, __LINE__);
            }
            log_warning("%s:%d: entity is not destructed\n", __FILE__, __LINE__);
        }
        return true;
    }

    bool run_config()
    {
        if (!this->isAllocated()) return false;

        // Set the object to deinitialize if it is not "dangerous" and if was requested.
        if (this->kill_me)
        {
			if (this->state > State::Constructing && this->state < State::DeInitializing)
            {
				this->state = State::DeInitializing;
            }

			this->kill_me = false;
        }

        bool result = true;
		switch (this->state)
        {
        default:
        case State::Invalid:
            result = false;
            break;

        case State::Constructing:
            result = this->config_ctor();
            break;

        case State::Initializing:
            result = this->config_init();
            break;

        case State::Active:
            result = this->config_active();
            break;

        case State::DeInitializing:
            this->config_deinit();
            break;

        case State::Destructing:
            this->config_dtor();
            break;

        case State::Waiting:
        case State::Terminated:
            /* Do nothing. */
            break;
        }

        if (!result)
        {
            this->setUpdateGUID(EGO_GUID_INVALID);
        }
        else if (State::Active == this->state)
        {
            this->setUpdateGUID(LISTTYPE::get().getUpdateGUID());
        }

        return result;
    }

	/// Begin turning off an entity.
	void POBJ_REQUEST_TERMINATE() {
		if (isAllocated() && State::Invalid != state) {
			if (State::Terminated != state) {
				kill_me = true;
			}
			on = false;
		}
	}

	void POBJ_END_SPAWN() {
		if (isAllocated()) {
			if (spawning) {
				spawning = false;
				Ego::Entities::spawnDepth--;
			}
		}
	}

	size_t GET_INDEX_POBJ(size_t FAIL_VALUE) const {
		return LAMBDA(!ALLOCATED_PBASE(), FAIL_VALUE, getIndex());
	}
	REFTYPE GET_REF_POBJ(REFTYPE FAIL_VALUE) const {
		return (REFTYPE)GET_INDEX_POBJ(FAIL_VALUE);
	}

};
