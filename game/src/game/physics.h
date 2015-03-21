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

/// @file game/physics.h

#pragma once

#include "game/egoboo_typedef.h"
#include "egolib/bbox.h"

//--------------------------------------------------------------------------------------------

namespace Physics
{
    struct Environment
    {

        /**
         * @brief
         *  Extra downhill force.
         * @default
         *  1.0f
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        float hillslide;

        /**
         * @brief
         *  Friction on tiles that are marked with MAPFX_SLIPPY.
         * @default
         *  1.0f
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        float slippyfriction;

        /**
         * @brief
         *  Air friction.
         * @default
         *  0.9868f
         * @remark
         *  0.9868f is approximately real world air friction.
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        float airfriction;

        /**
         * @brief
         *  Ice friction.
         * @default
         *  0.9738f (square of air friction)
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        float icefriction;

        /**
         * @brief
         *  Water friction.  
         * @default
         *  0.8f
         * @todo     
         *  Short description of (reasonable) limits and effect.
         */
        float waterfriction;

        /**
         * @brief
         *  Friction on tiles that are not marked with MAPFX_SLIPPY.
         * @default
         *  0.91f
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        float noslipfriction;

        /**
         * @brief
         *  Gravitational force.
         * @default
         *  -1.0f
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        float gravity;

        /**
         * @brief
         *  The game's windspeed.
         * @default
         *  <tt>(0,0,0)</tt>
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        fvec3_t windspeed;

        /**
         * @brief
         *  The game's waterspeed.
         * @default
         *  <tt>(0,0,0)</tt>
         * @todo
         *  Short description of (reasonable) limits and effect.
         */
        fvec3_t waterspeed;

        /**
         * @brief
         *  Construct this environment with its default values.
         */
        Environment() :
            hillslide(1.0f),
            slippyfriction(1.0f),
            airfriction(0.9868f),
            waterfriction(0.80f),
            icefriction(0.9738f),
            noslipfriction(0.91),
            gravity(-1.0f),
            windspeed(),
            waterspeed()
        {}

    };

    extern Environment g_environment;
}

//--------------------------------------------------------------------------------------------

class Object;
struct prt_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define PLATTOLERANCE       50                     ///< Platform tolerance...

enum
{
    PHYS_PLATFORM_NONE = 0,
    PHYS_PLATFORM_OBJ1 = ( 1 << 0 ),
    PHYS_PLATFORM_OBJ2 = ( 1 << 1 )
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct orientation_t
{
    FACING_T facing_z;            ///< Character's z-rotation 0 to 0xFFFF
    FACING_T map_twist_facing_y;  ///< Character's y-rotation 0 to 0xFFFF
    FACING_T map_twist_facing_x;  ///< Character's x-rotation 0 to 0xFFFF
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/**
 * @brief
 *  Tracks the extrema and the sum of translations.
 * @remark
 *  Given a set of translation vectors \f$t_0,t_1,t_2,\ldots,t_n\f$
 *  the minimum of the translations is defined as
 *  \f[
 *  \left(
 *  min(t_{0_x},\ldots,t_{n_x}),
 *  min(t_{0_y},\ldots,t_{n_y}),
 *  min(t_{0_z},\ldots,t_{n_z})
 *  \right)
 *  \f]
 *  and the maximum as
 *  \f[
 *  \left(
 *  max(t_{0_x},\ldots,t_{n_x}),
 *  max(t_{0_y},\ldots,t_{n_y}),
 *  max(t_{0_z},\ldots,t_{n_z})
 *  \right)
 *  \f]
 *  The sum of the translations is
 *  \[
 *  \sum_{i=0}^n t_i
 *  \]
 *  If the set of translations is empty, the minimum, the maximum and the sum are all 0.
 */
struct apos_t
{
    
    /**
     * @brief
     *  The minimum of the translations.
     * @default
     *  <tt>(0,0,0)</tt>
     */
    fvec3_t mins;
    
    /**
     * @brief
     *  The maximum of the translations.
     * @default
     *  <tt>(0,0,0)</tt>
     */
    fvec3_t maxs;

    /**
     * @brief
     *  The translation induced by the translation sequence.
     * @default
     *  <tt>(0,0,0)</tt>
     */
    fvec3_t sum;

	apos_t() :
		mins(),
		maxs(),
		sum()
	{
		//ctor
	}

    apos_t(const apos_t& other) :
        mins(other.mins),
        maxs(other.maxs),
        sum(other.sum)
    {
    }

    apos_t& operator=(const apos_t& other)
    {
        mins = other.mins;
        maxs = other.maxs;
        sum  = other.sum;
        return *this;
    }

    static void ctor(apos_t *self)
    {
        if (!self)
        {
            throw std::invalid_argument("nullptr == self");
        }
        self->mins = fvec3_t::zero;
        self->maxs = fvec3_t::zero;
        self->sum  = fvec3_t::zero;
    }


    static apos_t *self_clear(apos_t *self)
    {
        if (!self)
        {
            return nullptr;
        }
        self->mins = fvec3_t::zero;
        self->maxs = fvec3_t::zero;
        self->sum  = fvec3_t::zero;
    
        return self;
    }

    static bool self_union(apos_t *self, const apos_t *other);
    static bool self_union(apos_t *self, const fvec3_t& other);
};


bool apos_self_union_index(apos_t * lhs, const float val, const int index );
bool apos_evaluate( const apos_t * src, fvec3_t& dst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Data for doing the physics in bump_all_objects()
/// @details should prevent you from being bumped into a wall
struct phys_data_t
{
    apos_t         aplat, acoll;
    fvec3_t        avel;

    float          bumpdampen;                    ///< "Mass" = weight / bumpdampen
    Uint32         weight;                        ///< Weight
    float          dampen;                        ///< Bounciness
};

phys_data_t *phys_data_clear(phys_data_t *self);
phys_data_t *phys_data_ctor(phys_data_t *self);

phys_data_t *phys_data_sum_aplat(phys_data_t *self, const fvec3_t& v);
phys_data_t *phys_data_sum_acoll(phys_data_t *self, const fvec3_t& v);
phys_data_t *phys_data_sum_avel(phys_data_t *self, const fvec3_t& v);

phys_data_t *phys_data_sum_aplat_index(phys_data_t *self, const float v, const int index);
phys_data_t *phys_data_sum_acoll_index(phys_data_t *self, const float v, const int index);
phys_data_t *phys_data_sum_avel_index(phys_data_t *self, const float v, const int index);

//--------------------------------------------------------------------------------------------
/**
 * @brief
 *  A "breadcrumb" associates various information with a position.
 */
struct breadcrumb_t
{
    bool valid;     ///< Is this position valid?
    fvec3_t pos;    ///< A stored safe position.
    uint32_t grid;  ///< The grid index of this position.
                    ///< @todo Should be GridIndex.
    float radius;   ///< The size of the object at this position.
    BIT_FIELD bits; ///< The collision bits of the object at this position.
                    ///< @todo Should certainly not be a float.
    uint32_t time;  ///< the time when the breadcrumb was created.
    uint32_t id;    ///< an id for differentiating the timing of several events at the same "time".

	breadcrumb_t() : 
		valid(false),
		pos(0.0f, 0.0f, 0.0f),
		grid(0),
		radius(0.0f),
		bits(0),
		time(0),
		id(0)
	{
		//ctor
	}

    void assign(const breadcrumb_t& other)
    {
        valid = other.valid;
        pos = other.pos;
        grid = other.grid;
        radius = other.radius;
        bits = other.bits;
        time = other.time;
        id = other.id;
    }
    
    void reset()
    {
        valid = false;
        pos = fvec3_t::zero;
        grid = 0;
        radius = 0.0f;
        bits = 0;
        time = 0;
        id = 0;
    }

    breadcrumb_t(const breadcrumb_t& other) :
        valid(other.valid),
        pos(other.pos),
        grid(other.grid),
        radius(other.radius),
        bits(other.bits),
        time(other.time),
        id(other.id)
    {
    }

    breadcrumb_t& operator=(const breadcrumb_t& other)
    {
        assign(other);
        return *this;
    }
    /**
     * @brief
     *  Get if a breadcrumb is older than another breadcrumb.
     * @param x
     *  the first breadcrumb
     * @param y
     *  the second breadcrumb
     * @return
     *  @a true if the first breadcrumb is older than the second breadcrumb
     */
    static bool isOlder(const breadcrumb_t& x,const breadcrumb_t& y)
    {
        return x.time < y.time;
    }

    /**
     * @brief
     *  Get if a breadcrumb is younger than another breadcrumb.
     * @param x
     *  the first breadcrumb
     * @param y
     *  the second breadcrumb
     * @return
     *  @a true if this breadcrumb is younger than the other breadcrumb
     */
    static bool isYounger(const breadcrumb_t& x,const breadcrumb_t& y)
    {
        return x.time > y.time;
    }

    static breadcrumb_t *init(breadcrumb_t *self, Object *object);
    static breadcrumb_t *init(breadcrumb_t *self, prt_t *particle);
    static bool cmp(const breadcrumb_t& x, const breadcrumb_t& y);
};



//--------------------------------------------------------------------------------------------

struct breadcrumb_list_t
{
	static const size_t MAX_BREADCRUMB = 32;

    bool on;
    size_t count;
    std::array<breadcrumb_t, MAX_BREADCRUMB> lst;

	breadcrumb_list_t() :
        on(false), 
        count(0),
        lst()
	{
	}

	/**
 	 * @brief
	 *	Compact this breadcrumb list.
	 */
	void compact()
	{
		if (!on) return;
		size_t total, valid;
		for (total = 0, valid = 0; total < count; ++total)
		{
			breadcrumb_t& source = lst[total];
			if (source.valid)
			{
				if (total != valid)
				{
					breadcrumb_t& target = lst[valid];
                    target = source;
				}
				valid++;
			}
		}
		count = valid;
	}

	/**
	 * @brief
	 *	Is this breadcrumb list empty.
	 * @return
	 *	@a true if this breadcrumb list is empty or "off", @a false otherwise
	 */
	bool empty() const
	{
		return (0 == count) || !on;
	}
	
	/**
	 * @brief
	 *	Is this breadcrumb list full.
	 * @return
	 *	@a true if this breadcrumb list is full or is "off", @a false otherwise.
	 */
	bool full() const
	{
		return (count >= MAX_BREADCRUMB) || !on;
	}

    static bool add(breadcrumb_list_t *self, breadcrumb_t *breadcrumb);
    static breadcrumb_t *last_valid(breadcrumb_list_t *self);

    /**
     * @brief
     *  Get the oldest, valid breadcrumb of the given grid index.
     * @param gridIndex
     *   the grid index
     * @return
     *  a pointer to the oldest, valid breadcrumb of the given grid index if one exists,
     *  @a nullptr otherwise
     */
    static breadcrumb_t *oldest_grid(breadcrumb_list_t *self, Uint32 gridIndex);
    /**
     * @brief
     *  Get the oldest, valid breadcrumb.
     * @return
     *  a pointer to the oldest, valid breadcrumb if one exists, @a nullptr otherwise
     */
    static breadcrumb_t *oldest(breadcrumb_list_t *self);
    /**
     * @brief
     *  Get the youngest, valid breadcrumb.
     * @return
     *  a pointer to the youngest, valid breadcrumb if one exists, @a nullptr otherwise
     */
    static breadcrumb_t *newest(breadcrumb_list_t *self);

protected:
    static void validate(breadcrumb_list_t *self);
    static breadcrumb_t *alloc(breadcrumb_list_t *self);
};




//--------------------------------------------------------------------------------------------
// the global physics/friction values

extern const float PLATFORM_STICKINESS;     ///< Friction between characters and platforms

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @details use the velocity of an object and its oct_bb_t to determine the
///               amount of territory that an object will cover in the range [tmin,tmax].
///               One update equals [tmin,tmax] == [0,1].
bool phys_expand_oct_bb(const oct_bb_t& src, const fvec3_t& vel, const float tmin, const float tmax, oct_bb_t& dst);
/// @details use the object velocity to figure out where the volume that the character will
///               occupy during this update. Use the loser chr_max_cv and include extra height if
///               it is a platform.
bool phys_expand_chr_bb(Object *pchr, float tmin, float tmax, oct_bb_t& dst);
bool phys_expand_prt_bb(prt_t *pprt, float tmin, float tmax, oct_bb_t& dst);

bool phys_estimate_collision_normal(const oct_bb_t& obb_a, const oct_bb_t& pobb_b, const float exponent, oct_vec_v2_t& odepth, fvec3_t& nrm, float& depth);
bool phys_estimate_pressure_normal(const oct_bb_t& obb_a, const oct_bb_t& pobb_b, const float exponent, oct_vec_v2_t& odepth, fvec3_t& nrm, float& depth);

bool phys_intersect_oct_bb(const oct_bb_t& src1, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t& src2, const fvec3_t& pos2, const fvec3_t& vel2, int test_platform, oct_bb_t& dst, float *tmin, float *tmax);

bool get_chr_mass(Object *pchr, float *wt);
bool get_prt_mass(prt_t *pprt, Object *pchr, float *wt);
void get_recoil_factors(float wta, float wtb, float * recoil_a, float * recoil_b);

/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_0(bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform);
bool test_interaction_1(const oct_bb_t& cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform);
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-character collisions
bool test_interaction_2(const oct_bb_t& cv_a, const fvec3_t& pos_a, const oct_bb_t& cv_b, const fvec3_t& pos_b, int test_platform);
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_close_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform);
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_close_1(const oct_bb_t& cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform);
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_close_2(const oct_bb_t& cv_a, const fvec3_t& pos_a, const oct_bb_t& cv_b, const fvec3_t& pos_b, int test_platform);

/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-particle collisions.
bool get_depth_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth);
/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-particle collisions.
bool get_depth_1(const oct_bb_t& cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth);
/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-character collisions
bool get_depth_2(const oct_bb_t& cv_a, const fvec3_t& pos_a, const oct_bb_t& cv_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth);
