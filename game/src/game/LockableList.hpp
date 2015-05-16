
/// @file game/LockableList.hpp
/// @warning A lockable list; temporarily used to refactor the list of enchants and the list of particles.

#pragma once

#include "egolib/egolib.h"

/** @todo Enforce that REFTYPE is an unsigned type.  */
template <typename TYPE, typename REFTYPE, REFTYPE INVALIDREF, size_t COUNT, bsp_type_t BSPTYPE>
struct _LockableList : public Id::NonCopyable
{
    _LockableList() :
        update_guid(EGO_GUID_INVALID),
        usedCount(0),
        freeCount(0),
        termination_count(0),
        termination_list(),
        activation_count(0),
        activation_list(),
        used_ref(),
        lst(),
        lockCount(0)
    {
       //ctor
    }

    virtual ~_LockableList()
    {
        //dtor
    }

    void reinit()
    {
        deinit();
        init();
    }

protected:
    Ego::GUID update_guid;
    int usedCount;
    int freeCount;

public:
    bool isValidRef(const REFTYPE ref) const
    {
        return ref < getCount();
    }

    bool push_used(const REFTYPE ref)
    {
        if (!isValidRef(ref))
        {
            return false;
        }

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
        if (find_used_ref(ref) != std::numeric_limits<size_t>::max())
        {
            return false;
        }
#endif

        EGOBOO_ASSERT(!lst[ref].obj_base.in_used_list);

        if (usedCount < getCount())
        {
            used_ref[usedCount] = ref;

            usedCount++;
            update_guid++;

            lst[ref].obj_base.in_used_list = true;

            return true;
        }

        return false;
    }

    Ego::GUID getUpdateGUID() const
    {
        return update_guid;
    }

    //--------------------------------------------------------------------------------------------
    void initialize()
    {
        // Initialize the list.
        init();

        // Construct the sub-objects.
        for (size_t i = 0; i < getCount(); ++i)
        {
            TYPE *x = &(lst[i]);

            // Blank out all the data, including the obj_base data.
            BLANK_STRUCT_PTR(x);

            // construct the base object
            POBJ_GET_PBASE(x)->ctor(x, BSPTYPE, i);

            // Construct the object.
            x->config_do_ctor();
        }
    }

    void uninitialize()
    {
        // Construct the sub-objects.
        for (size_t i = 0; i < getCount(); ++i)
        {
            TYPE *x = &(lst[i]);

            // Destruct the object
            x->config_do_dtor();

            // Destruct the entity.
            POBJ_GET_PBASE(x)->dtor();
        }

        // DeInitialize the list.
        deinit();
    }

protected:
    void init()
    {
        clear();

        // Add the objects to the free list.
        for (REFTYPE ref = 0; ref < getCount(); ++ref)
        {
            add_free_ref(ref);
        }
    }

    void deinit()
    {
        // Request that the sub-objects destroy themselves.
        for (size_t i = 0; i < getCount(); ++i)
        {
            TYPE::config_deconstruct(get_ptr(i), 100);
        }
        clear();
    }

public:
    void clear()
    {
        // Clear out the free and used lists.
        freeCount = 0;
        usedCount = 0;
        for (size_t i = 0; i < getCount(); ++i)
        {
            // Blank out the list entries.
            free_ref[i] = INVALIDREF;
            used_ref[i] = INVALIDREF;

            // Let the entities know that they are not in a list.
            lst[i].obj_base.in_free_list = false;
            lst[i].obj_base.in_used_list = false;
        }
    }

protected:
    // List of references to TYPEs which requested termination
    // while the particle list was locked.
    size_t  termination_count;
    REFTYPE termination_list[COUNT];

    // List of reference to TYPEs which requested activation
    // while the particle list was locked.
    size_t  activation_count;
    REFTYPE activation_list[COUNT];

public:
    REFTYPE used_ref[COUNT];

protected:
    REFTYPE free_ref[COUNT];
    TYPE lst[COUNT];

public:
    TYPE *get_ptr(const size_t index)
    {
        return LAMBDA(index >= COUNT, nullptr, lst + index);
    }
    void lock()
    {
        lockCount++;
    }
    void unlock()
    {
        lockCount--;
    }
    size_t getCount() const
    {
        return COUNT;
    }
    int getLockCount() const
    {
        return lockCount;
    }
    int getUsedCount() const
    {
        return usedCount;
    }
    int getFreeCount() const
    {
        return freeCount;
    }

    /// Put the reference into the activation list so that the referenced object can be
    // activated right after the loop is completed.
    bool add_activation(const REFTYPE ref)
    {
        bool retval = false;

        if (!isValidRef(ref)) return false;

        if (activation_count < getCount())
        {
            activation_list[activation_count] = ref;
            activation_count++;

            retval = true;
        }

        lst[ref].obj_base.turn_me_on = true;

        return retval;
    }


    /// Put the reference into the activation list so that the referenced object can be
    // terminated right after the loop is completed.
    bool add_termination(const REFTYPE ref)
    {
        bool retval = false;

        if (!isValidRef(ref)) return false;

        if (termination_count < getCount())
        {
            termination_list[termination_count] = ref;
            termination_count++;

            retval = true;
        }

        // Mark the object as "waiting to be terminated".
        POBJ_REQUEST_TERMINATE(get_ptr(ref));

        return retval;
    }

    /// @brief Move all objects to the free list
    void free_all()
    {
        for (REFTYPE ref = 0; ref < getCount(); ++ref)
        {
            free_one(ref);
        }
    }

    /**
     * @brief
     *	Run all deferred updates if the list is not locked.
     */
    void maybeRunDeferred()
    {
        // Go through the list and activate all the objects that
        // were created while the list was iterating.
        for (size_t i = 0; i < activation_count; ++i)
        {
            REFTYPE ref = activation_list[i];

            if (!ALLOCATED(ref)) continue;
            TYPE *x = get_ptr(ref);

            if (!x->obj_base.turn_me_on) continue;

            x->obj_base.on = true;
            x->obj_base.turn_me_on = false;
        }
        activation_count = 0;

        // Go through and delete any objects that were
        // supposed to be deleted while the list was iterating
        for (size_t i = 0; i < termination_count; ++i)
        {
            free_one(termination_list[i]);
        }
        termination_count = 0;
    }

protected:
    /// @brief Prune the free list.
    void prune_free_list()
    {
        for (size_t i = 0; i < freeCount; ++i)
        {
            bool removed = false;

            REFTYPE ref = free_ref[i];

            if (isValidRef(ref) && INGAME_BASE(ref))
            {
                removed = remove_free_idx(i);
            }

            if (removed && !lst[ref].obj_base.in_free_list)
            {
                push_used(ref);
            }
        }
    }

    /// @brief Prune the used list.
    void prune_used_list()
    {
        // prune the used list
        for (size_t i = 0; i < usedCount; ++i)
        {
            bool removed = false;

            REFTYPE ref = used_ref[i];

            if (!isValidRef(ref) || !DEFINED(ref))
            {
                removed = remove_used_idx(i);
            }

            if (removed && !lst[ref].obj_base.in_free_list)
            {
                add_free_ref(ref);
            }
        }
    }

public:
    /// @brief Stick an object back into the free object list.
    bool free_one(const REFTYPE ref)
    {
        // Ensure that we have a valid reference.
        if (!isValidRef(ref)) return false;
        TYPE *obj = get_ptr(ref);

        // If the object is not allocated (i.e. in the state range ["constructing","destructing"])
        // then its reference is in the free list.
        if (!ALLOCATED(obj)) return false;

        Ego::Entity *parentObj = POBJ_GET_PBASE(obj);

        // If we are inside an iteration, do not actually change the length of the list.
        // This would invalidate all iterators.
        if (getLockCount() > 0)
        {
            return add_termination(ref);
        }
        else
        {
            // Ensure that the entity reaches the "destructing" state.
            // @todo This is redundant.
            obj = TYPE::config_deinitialize(obj, 100);
            if (!obj) return false;
            // Ensure that the entity reaches the "terminated" state.
            obj = TYPE::config_deconstruct(obj, 100);
            if (!obj) return false;

            if (parentObj->in_used_list)
            {
                remove_used_ref(ref);
            }

            if (parentObj->in_free_list)
            {
                return true;
            }
            else
            {
                return add_free_ref(ref);
            }
        }
    }

protected:
    bool remove_free_ref(const REFTYPE ref)
    {
        // Find the reference in the free list.
        size_t index = find_free_ref(ref);
        // If the object is not in the free list ...
        if (std::numeric_limits<size_t>::max() != index)
        {
            // ... do nothing.
            return false;
        }
        return remove_free_idx(index);
    }

    bool remove_used_ref(const REFTYPE ref)
    {
        // Find the object in the used list.
        size_t index = find_used_ref(ref);
        // If it is not in the used list ...
        if (std::numeric_limits<size_t>::max() == index)
        {
            // ... do nothing.
            return false;
        }
        // Otherwise free the object.
        return remove_used_idx(index);
    }

    bool remove_free_idx(const size_t index)
    {
        // Is the index within bounds?
        if (index >= freeCount) return false;

        REFTYPE ref = free_ref[index];

        // Blank out the reference in the list.
        free_ref[index] = INVALIDREF;

        if (isValidRef(ref))
        {
            // let the object know it is not in the list anymore
            lst[ref].obj_base.in_free_list = false;
        }

        // Shorten the list.
        freeCount--;
        update_guid++;

        // Fast removal by swapping the deleted element with the last element.
        if (freeCount > 0)
        {
            std::swap(free_ref[index], free_ref[freeCount]);
        }

        return true;
    }

    bool add_free_ref(const REFTYPE ref)
    {
        bool retval;

        if (!isValidRef(ref)) return false;

    #if defined(_DEBUG)
        if (find_free_ref(ref) != std::numeric_limits<size_t>::max())
        {
            return false;
        }
    #endif

        EGOBOO_ASSERT(!lst[ref].obj_base.in_free_list);

        retval = false;
        if (freeCount < getCount())
        {
            free_ref[freeCount] = ref;

            freeCount++;
            update_guid++;

            lst[ref].obj_base.in_free_list = true;

            retval = true;
        }

        return retval;
    }

    /**
     * @brief
     *  If the reference exists in the used list, return its index in the used list.
     * @return
     *  the index of the reference in the used list if it exists,
     *  std::numeric_limits<size_t>::max() otherwise
     */
    size_t find_used_ref(const REFTYPE ref)
    {
        if (!isValidRef(ref))
        {
            return std::numeric_limits<size_t>::max();
        }
        for (size_t i = 0; i < usedCount; ++i)
        {
            if (ref == used_ref[i])
            {
                EGOBOO_ASSERT(lst[ref].obj_base.in_used_list);
                return i;
            }
        }

        return std::numeric_limits<size_t>::max();
    }

    /**
     * @brief
     *  If the reference exists in the free list, return its index in the free list.
     * @return
     *  the index of the reference in the free list if it exists,
     *  std::numeric_limits<size_t>::max() otherwise
     */
    size_t find_free_ref(const REFTYPE ref)
    {
        if (!isValidRef(ref))
        {
            std::numeric_limits<size_t>::max();
        }
        for (size_t i = 0; i < freeCount; ++i)
        {
            if (ref == free_ref[i])
            {
                EGOBOO_ASSERT(lst[ref].obj_base.in_free_list);
                return i;
            }
        }

        return std::numeric_limits<size_t>::max();
    }

    bool remove_used_idx(const size_t index)
    {
        // Valid used list index?
        if (index >= usedCount) return false;

        REFTYPE ref = used_ref[index];

        // Blank out the index in the used list.
        used_ref[index] = INVALIDREF;

        if (isValidRef(ref))
        {
            // Let the object know it is not in the list anymore.
            lst[ref].obj_base.in_used_list = false;
        }

        // Shorten the list.
        usedCount--;
        update_guid++;

        // Fast removal by swapping with the last element in the list.
        if (usedCount > 0)
        {
            std::swap(used_ref[index], used_ref[usedCount]);
        }

        return true;
    }

    /**
     * @brief
     *  Pop an unused reference from the free list.
     * @return
     *  the unused reference if it exists, INVALIDREF otherwise
     */
    virtual REFTYPE pop_free()
    {
        REFTYPE ref = INVALIDREF;
        size_t loops = 0;

        while (freeCount > 0)
        {
            freeCount--;
            update_guid++;

            ref = free_ref[freeCount];

            // Completely remove it from the free list
            free_ref[freeCount] = INVALIDREF;

            if (isValidRef(ref))
            {
                // Let the object know it is not in the free list any more.
                lst[ref].obj_base.in_free_list = false;
                break;
            }

            loops++;
        }

        if (loops > 0)
        {
            log_warning("%s - there is something wrong with the free stack. %" PRIuZ " loops.\n", __FUNCTION__, loops);
        }

        return ref;
    }
protected:
    int lockCount;

public:
    
    /**
     * @brief
     *  Get if an object is in-game.
     * @param ptr
     *  the object
     * @return
     *  @a true if the object is in-game, @a false otherwise
     */
    bool INGAME_BASE_RAW(const TYPE *ptr)
    {
        return ACTIVE_PBASE(POBJ_GET_PBASE(ptr))
            && ON_PBASE(POBJ_GET_PBASE(ptr));
    }

    /**
     * @brief
     *  Get if an object is defined.
     * @param ptr
     *  the object
     * @return
     *  @a true if the object is defined, @a false otherwise
     */
    bool DEFINED_BASE_RAW(const TYPE *ptr)
    {
        return ALLOCATED_PBASE(POBJ_GET_PBASE(ptr))
            && !TERMINATED_PBASE(POBJ_GET_PBASE(ptr));
    }

    /**
     * @brief
     *  Get if an object is allocated.
     * @param ptr
     *  the object
     * @return
     *  @a true if the object is allocated, @a false otherwise
     */
    bool ALLOCATED_BASE_RAW(const TYPE *ptr)
    {
        return ALLOCATED_PBASE(POBJ_GET_PBASE(ptr));
    }

    /**
     * @brief
     *  Get if an object is active.
     * @param ptr
     *  the object
     * @return
     *  @a true if the object is active, @a false otherwise
     */
    bool ACTIVE_BASE_RAW(const TYPE *ptr)
    {
        return ACTIVE_PBASE(POBJ_GET_PBASE(ptr));
    }

    /**
     * @brief
     *  Get if an object is waiting.
     * @param ptr
     *  the object
     * @return
     *  @a true if the object is waiting, @a false otherwise
     */
    bool WAITING_BASE_RAW(const TYPE *ptr)
    {
        return WAITING_PBASE(POBJ_GET_PBASE(ptr));
    }

    /**
     * @brief
     *  Get if an object is terminated.
     * @param ptr
     *  the object
     * @return
     *  @a true if the object is terminated, @a false otherwise
     */
    bool TERMINATED_BASE_RAW(const TYPE *ptr)
    {
        return TERMINATED_PBASE(POBJ_GET_PBASE(ptr));
    }

public:
    /**
     * @brief
     *  Get if a reference refers to a defined object.
     * @param ref
     *  the reference
     * @return
     *  @a true if the reference is valid and refers to a defined object, @a false otherwise
     */
    bool DEFINED(const REFTYPE ref)
    {
        return isValidRef(ref)
            && DEFINED_BASE_RAW(get_ptr(ref));
    }

    /**
     * @brief
     *  Get if a reference refers to an active object.
     * @param ref
     *  the reference
     * @return
     *  @a true if the reference is valid and refers to an active object, @a false otherwise
     */
    bool ACTIVE(const REFTYPE ref)
    {
        return isValidRef(ref)
            && ACTIVE_BASE_RAW(get_ptr(ref));
    }

    /**
     * @brief
     *  Get if a reference refers to a waiting object.
     * @param ref
     *  the reference
     * @return
     *  @a true if the reference is valid and refers to a waiting object, @a false otherwise
     */
    bool WAITING(const REFTYPE ref)
    {
        return isValidRef(ref)
            && WAITING_BASE_RAW(get_ptr(ref));
    }

    /**
     * @brief
     *  Get if a reference refers to an allocated object.
     * @param ref
     *  the reference
     * @return
     *  @a true if the reference is valid and refers to an allocated object, @a false otherwise
     */
    bool ALLOCATED(const REFTYPE ref)
    {
        return isValidRef(ref)
            && ALLOCATED_BASE_RAW(get_ptr(ref));
    }

    /**
     * @brief
     *  Get if a reference refers to a terminated object.
     * @param ref
     *  the reference
     * @return
     *  @a true if the reference is valid and refers to a terminated object, @a false otherwise
     */
    bool TERMINATED(const REFTYPE ref)
    {
        return isValidRef(ref)
            && TERMINATED_BASE_RAW(get_ptr(ref));
    }

    /**
     * @brief
     *  Get if an object pointer refers to a valid object.
     * @param ptr
     *  the pointer
     * @return
     *  @a true if @a ptr is not @a nullptr and refers to an object with a valid reference,
     *  @a false otherwise
     */
    bool VALID_PTR(const TYPE *ptr)
    {
        return (nullptr != ptr)
            && isValidRef(GET_REF_POBJ(ptr, INVALIDREF));
    }

    bool DEFINED(const TYPE *ptr)
    {
        return VALID_PTR(ptr)
            && DEFINED_BASE_RAW(ptr);
    }

    bool ALLOCATED(const TYPE *ptr)
    {
        return VALID_PTR(ptr)
            && ALLOCATED_BASE_RAW(ptr);
    }

    bool ACTIVE(const TYPE *ptr)
    {
        return VALID_PTR(ptr)
            && ACTIVE_BASE_RAW(ptr);
    }

    bool WAITING(const TYPE *ptr)
    {
        return VALID_PTR(ptr)
            && WAITING_BASE_RAW(ptr);
    }

    bool TERMINATED(const TYPE *ptr)
    {
        return VALID_PTR(ptr)
            && TERMINATED_BASE_RAW(ptr);
    }

    bool INGAME_BASE(const REFTYPE ref)
    {
        return isValidRef(ref)
            && INGAME_BASE_RAW(get_ptr(ref));
    }

    bool INGAME_BASE(const TYPE *ptr)
    {
        return VALID_PTR(ptr)
            && INGAME_BASE_RAW(ptr);
    }

};
