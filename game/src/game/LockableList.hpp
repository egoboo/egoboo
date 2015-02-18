
/// @file game/LockableList.hpp
/// @warning A lockable list; temporarily used to refactor the list of enchants and the list of particles.

#pragma once

#include "egolib/egolib.h"

/** @todo Enforce that REFTYPE is an unsigned type.  */
template <typename TYPE, typename REFTYPE, REFTYPE INVALIDREF, size_t COUNT, bsp_type_t BSPTYPE>
struct _LockableList
{
    _LockableList()
    {
        update_guid = INVALID_UPDATE_GUID;
        usedCount = 0;
        freeCount = 0;
        lockCount = 0;
        termination_count = 0;
        activation_count = 0;
    }
    void reinit()
    {
        deinit();
        init();
    }

protected:
    unsigned update_guid;
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

    unsigned getUpdateGUID() const
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
            TYPE *x = lst + i;

            // Blank out all the data, including the obj_base data.
            BLANK_STRUCT_PTR(x);

            // construct the base object
            POBJ_GET_PBASE(x)->ctor(x, BSPTYPE, i);

            // Construct the object.
            x->ctor();
        }
    }

    void uninitialize()
    {
        // Construct the sub-objects.
        for (size_t i = 0; i < getCount(); ++i)
        {
            TYPE *x = lst + i;

            // Destruct the object
            x->dtor();

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
        for (size_t i = 0; i < getCount(); i++)
        {
            REFTYPE ref = i;
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

    bool request_terminate(const REFTYPE ref)
    {
        TYPE *obj = get_ptr(ref);
        EGOBOO_ASSERT(nullptr != obj);
        return TYPE::request_terminate(obj);
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

    /// @brief Reset all particles.
    void free_all()
    {
        for (REFTYPE ref = 0; ref < getCount(); ++ref)
        {
            free_one(ref);
        }
    }

protected:
    virtual bool free_one(const REFTYPE ref) = 0;

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
            log_warning("%s - there is something wrong with the free stack. %lu loops.\n", __FUNCTION__, loops);
        }

        return ref;
    }
protected:
    int lockCount;
};
