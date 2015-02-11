
/// @file game/LockableList.hpp
/// @warning A lockable list; temporarily used to refactor the list of enchants and the list of particles.

#pragma once

#include "egolib/egolib.h"

template <typename TYPE, size_t COUNT>
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
protected:
    int lockCount;
};

#define DEFINE_LOCKABLELIST_TYPE(TYPE, NAME, COUNT) \
	typedef _LockableList<TYPE,COUNT> s_c_list__##TYPE__##NAME;

#define DECLARE_LOCKABLELIST_EXTERN(TYPE, NAME, COUNT)   \
    DEFINE_LOCKABLELIST_TYPE(TYPE, NAME, COUNT);         \
    void   NAME##_ctor();                                \
    void   NAME##_dtor();                                \
    bool NAME##_push_used(const REF_T);                  \
    extern s_c_list__##TYPE__##NAME NAME

#define INSTANTIATE_LOCKABLELIST_STATIC(TYPE, NAME, COUNT) \
    static s_c_list__##TYPE__##NAME NAME;

#define INSTANTIATE_LOCKABLELIST(ACCESS,TYPE,NAME, COUNT) \
    ACCESS s_c_list__##TYPE__##NAME NAME;

#ifndef IMPLEMENT_LOCKABLELIST
#define IMPLEMENT_LOCKABLELIST(TYPE, NAME, COUNT)         \
    static int NAME##_find_free_ref(const REF_T); \
    static bool NAME##_push_free(const REF_T);    \
    static size_t  NAME##_pop_free(const int);    \
    static int NAME##_find_used_ref(const REF_T); \
    static size_t NAME##_pop_used(const int);
#endif
