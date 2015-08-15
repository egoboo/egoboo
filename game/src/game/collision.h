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

//Forward declarations
class Object;

/// element for storing pair-wise "collision" data
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
    uint32_t  tileb;

    // Some information about the estimated collision.
    float tmin, tmax;
    oct_bb_t cv;

	static bool cmp(const CoNode_t &self, const CoNode_t &other);
	static int cmp_unique(const CoNode_t &self, const CoNode_t &other);
};

static inline bool operator == (const CoNode_t &lhs, const CoNode_t &rhs) {
    return CoNode_t::cmp_unique(lhs, rhs);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct CollisionSystem : public Id::NonCopyable
{
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

};

void bump_all_objects();