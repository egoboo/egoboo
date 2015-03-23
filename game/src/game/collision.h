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

/// @file game/collision.h

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

#define CHR_MAX_COLLISIONS       (MAX_CHR*8 + MAX_PRT)
#define COLLISION_HASH_NODES     (CHR_MAX_COLLISIONS*2)
#define COLLISION_LIST_SIZE      256

class Object;
struct prt_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct CoNode_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// element for storing pair-wise "collision" data
/// @note this does not use the "standard" method of inheritance from hash_node_t, where an
/// instance of hash_node_t is embedded inside CoNode_t as CoNode_t::base or something.
/// Instead, a separate lists of free hash_nodes and free CoNodes are kept, and they are
/// associated through the hash_node_t::data pointer when the hash node is added to the
/// hash_list_t
struct CoNode_t
{
public:
    CoNode_t();

    // The "colliding" objects.
    CHR_REF chra;
    PRT_REF prta;

    // The "collided with" objects.
    CHR_REF chrb;
    PRT_REF prtb;
    Uint32  tileb;

    // Some information about the estimated collision.
    float tmin, tmax;
    oct_bb_t cv;

    static CoNode_t *ctor(CoNode_t *self);
	static int matches(const CoNode_t *self, const CoNode_t *other);
	/** @todo This test is broken or has dead code. */
	static int cmp(const CoNode_t *self, const CoNode_t *other);
	static int cmp_unique(const CoNode_t *self, const CoNode_t *other);
	static Uint8 generate_hash(const CoNode_t *self);
};

//--------------------------------------------------------------------------------------------

struct CoHashList_t : public hash_list_t
{
	CoHashList_t(size_t initialCapacity)
		: hash_list_t(initialCapacity)
	{}
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// global functions

// A temporary magazine allocator supporting the operations "acquire" and "refill".
template <typename Type,size_t Capacity>
struct Magazine
{
public:

    void reset()
    {
        _size = Capacity;
    }

    Type *acquire()
    {
        if (!_size) throw std::underflow_error("magazine underflow");
        return _elements[--_size];
    }

    Magazine() :
        _size(Capacity),
        _elements()
    {
        size_t index;
        try
        {
            for (index = 0; index < Capacity; ++index)
            {
                _elements[index] = new Type();
            }
        }
        catch (std::exception& ex)
        {
            while (index > 0)
            {
                delete _elements[--index];
                _elements[index] = nullptr;
            }
            _size = 0;
        }
    }

    virtual ~Magazine()
    {
        if (_size != Capacity)
        {
            throw std::runtime_error("invalid magazine state");
        }
        for (size_t index = 0; index < Capacity; ++index)
        {
            delete _elements[index];
            _elements[index] = nullptr;
        }
    }

    //Disable copying class
    Magazine(const Magazine& copy) = delete;
    Magazine& operator=(const Magazine&) = delete;

private:
    size_t _size;
    std::array<Type*, Capacity> _elements;
};

struct CollisionSystem
{
public:
    typedef Magazine < hash_node_t, COLLISION_HASH_NODES > HashNodeAry;
    typedef Magazine < CoNode_t, CHR_MAX_COLLISIONS > CollNodeAry;

protected:
    /**
     * @brief
     *  The collision system singleton.
     */
    static CollisionSystem *_singleton;
    /**
     * @brief
     *  Construct this collision system.
     * @remark
     *  Intentionally protected.
     */
    CollisionSystem();
    /**
     * @brief
     *  Destruct this collision system.
     * @remark
     *  Intentionally protected.
     */
    virtual ~CollisionSystem();

public:
    /// Magazine of hash nodes.
    HashNodeAry _hn_ary_2;
    
    /// Magazine of collision nodes.
    CollNodeAry _cn_ary_2;

public:
    CoHashList_t *_hash;
    Ego::DynamicArray<BSP_leaf_t *> _coll_leaf_lst;
    Ego::DynamicArray<CoNode_t> _coll_node_lst;


    static bool initialize();
    static CollisionSystem *get()
    {
        if (!CollisionSystem::_singleton)
        {
            throw std::runtime_error("collision system not initialized");
        }
        return CollisionSystem::_singleton;
    }
    static void uninitialize();

    void reset();
    
    //Disable copying class
    CollisionSystem(const CollisionSystem& copy) = delete;
    CollisionSystem& operator=(const CollisionSystem&) = delete;
};

/// Insert a collision into a collision hash list if it does not exist yet.
/// @param self the collision hash list
/// @param collision the collision
/// @param collisions the list of collisions
/// @param hashNodes the list of hash nodes
bool CoHashList_insert_unique(CoHashList_t *coHashList, CoNode_t *coNode, CollisionSystem::CollNodeAry& collNodeAry, CollisionSystem::HashNodeAry& hashNodeAry);


void bump_all_objects();