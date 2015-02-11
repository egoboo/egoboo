
/// @file game/LockableList.hpp
/// @warning A lockable list; temporarily used to refactor the list of enchants and the list of particles.

#pragma once

#include "egolib/egolib.h"

/** @todo Enforce that REFTYPE is an unsigned type.  */
template <typename TYPE, typename REFTYPE, size_t COUNT>
struct _LockableList
{
    _LockableList()
    {
        update_guid = INVALID_UPDATE_GUID;
        usedCount = 0;
        freeCount = 0;
        lockCount = 0;
    }
    unsigned update_guid;
    int usedCount;
    int freeCount;

    size_t used_ref[COUNT];
    size_t free_ref[COUNT];
    TYPE lst[COUNT];
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
    
    bool isInRange(REFTYPE index) const
    {
        return index < COUNT;
    }
protected:
    int lockCount;
};

#define DECLARE_LOCKABLELIST_EXTERN(TYPE, REFTYPE, NAME, COUNT)         \
    typedef _LockableList<TYPE,REFTYPE,COUNT> s_c_list__##TYPE__##NAME; \
    extern s_c_list__##TYPE__##NAME NAME;

#define INSTANTIATE_LOCKABLELIST(TYPE, REFTYPE, NAME, COUNT) \
    s_c_list__##TYPE__##NAME NAME;
