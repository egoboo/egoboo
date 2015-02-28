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
struct apos_t
{
	apos_t() :
		mins(),
		maxs(),
		sum()
	{
		//ctor
	}

    fvec3_t mins;
    fvec3_t maxs;
    fvec3_t sum;
};

bool apos_self_union( apos_t * lhs, apos_t * rhs );
bool apos_self_union_fvec3( apos_t * lhs, const fvec3_t& rhs );
bool apos_self_union_index( apos_t * lhs, const float val, const int index );
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

phys_data_t * phys_data_clear( phys_data_t * pphys );
phys_data_t * phys_data_ctor( phys_data_t * pphys );

phys_data_t * phys_data_sum_aplat( phys_data_t * pphys, const fvec3_t& vec );
phys_data_t * phys_data_sum_acoll( phys_data_t * pphys, const fvec3_t& vec );
phys_data_t * phys_data_sum_avel( phys_data_t * pphys, const fvec3_t& vec );

phys_data_t * phys_data_sum_aplat_index( phys_data_t * pphys, const float val, const int index );
phys_data_t * phys_data_sum_acoll_index( phys_data_t * pphys, const float val, const int index );
phys_data_t * phys_data_sum_avel_index( phys_data_t * pphys, const float val, const int index );

//--------------------------------------------------------------------------------------------
struct breadcrumb_t
{
    bool valid;     /// is this position valid
    fvec3_t pos;    ///< A stored safe position
    Uint32 grid;    ///< the grid index of this position
    float radius;   ///< the size of the object at this position
    float bits;     ///< the collision buts of the object at this position
    Uint32 time;    ///< the time when the breadcrumb was created
    Uint32 id;      ///< an id for differentiating the timing of several events at the same "time"

	breadcrumb_t() : 
		valid(false),
		pos(0.0f, 0.0f, 0.0f),
		grid(0),
		radius(0.0f),
		bits(0.0f),
		time(0),
		id(0)
	{
		//ctor
	}
};

breadcrumb_t * breadcrumb_init_chr( breadcrumb_t * bc, Object * pchr );
breadcrumb_t * breadcrumb_init_prt( breadcrumb_t * bc, prt_t * pprt );

int breadcrumb_cmp( const void * lhs, const void * rhs );

//--------------------------------------------------------------------------------------------

struct breadcrumb_list_t
{
	static const size_t MAX_BREADCRUMB = 32;
    bool on;
    int count;
    breadcrumb_t lst[MAX_BREADCRUMB];

	breadcrumb_list_t()
		: on(false), count(0)
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
			breadcrumb_t *source = lst + total;
			if (source->valid)
			{
				if (total != valid)
				{
					breadcrumb_t *target = lst + valid;
					memcpy(target, source, sizeof(breadcrumb_t));
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
#if 0
    /**
     * @brief
     *	Compact this breadcrumb list.
     * @param self
     *	this breakcrumb list
     */
    static void compact(breadcrumb_list_t *self);
#endif

    static void validate(breadcrumb_list_t *self);
    static bool add(breadcrumb_list_t *self, breadcrumb_t *breadcrumb);
    static breadcrumb_t *last_valid(breadcrumb_list_t *self);
    static breadcrumb_t *alloc(breadcrumb_list_t *self);
    static breadcrumb_t *oldest_grid(breadcrumb_list_t *self, Uint32 match_grid);
    static breadcrumb_t *oldest(breadcrumb_list_t *self);
    static breadcrumb_t *newest(breadcrumb_list_t *self);

};




//--------------------------------------------------------------------------------------------
// the global physics/friction values

#define STANDARD_GRAVITY -1.0f              ///< The ordinary amount of gravity

extern float   hillslide;                   ///< Extra downhill force
extern float   airfriction;                 ///< 0.9868 is approximately real world air friction
extern float   waterfriction;               ///< Water resistance
extern float   slippyfriction;              ///< Friction on tiles that are marked with MAPFX_SLIPPY
extern float   noslipfriction;              ///< Friction on normal tiles
extern float   gravity;                     ///< Gravitational accel
extern fvec3_t windspeed;                   ///< The game's windspeed
extern fvec3_t waterspeed;                  ///< The game's waterspeed

extern const float air_friction;            ///< gives the same terminal velocity in terms of the size of the game characters
extern const float ice_friction;            ///< estimte if the friction on ice
extern const float PLATFORM_STICKINESS;     ///< Friction between characters and platforms

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @details use the velocity of an object and its oct_bb_t to determine the
///               amount of territory that an object will cover in the range [tmin,tmax].
///               One update equals [tmin,tmax] == [0,1].
bool phys_expand_oct_bb( const oct_bb_t * src, const fvec3_t& vel, const float tmin, const float tmax, oct_bb_t * pdst );
/// @details use the object velocity to figure out where the volume that the character will
///               occupy during this update. Use the loser chr_max_cv and include extra height if
///               it is a platform.
bool phys_expand_chr_bb( Object * pchr, float tmin, float tmax, oct_bb_t * pdst );
bool phys_expand_prt_bb( prt_t * pprt, float tmin, float tmax, oct_bb_t * pdst );

bool phys_estimate_collision_normal( const oct_bb_t * pobb_a, const oct_bb_t * pobb_b, const float exponent, oct_vec_t * podepth, fvec3_t& nrm, float * tmin );
bool phys_estimate_pressure_normal( const oct_bb_t * pobb_a, const oct_bb_t * pobb_b, const float exponent, oct_vec_t * podepth, fvec3_t& nrm, float * depth );

bool phys_intersect_oct_bb( const oct_bb_t * src1, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t * src2, const fvec3_t& pos2, const fvec3_t& vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax );

bool get_chr_mass( Object * pchr, float * wt );
bool get_prt_mass( prt_t * pprt, Object * pchr, float * wt );
void get_recoil_factors( float wta, float wtb, float * recoil_a, float * recoil_b );

//Inline below
apos_t * apos_self_clear( apos_t * val );

/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform );
bool test_interaction_1( const oct_bb_t * cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform );
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-character collisions
bool test_interaction_2( const oct_bb_t * cv_a, const fvec3_t& pos_a, const oct_bb_t * cv_b, const fvec3_t& pos_b, int test_platform );
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_close_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform );
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_close_1( const oct_bb_t * cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform );
/// @brief Test whether two objects could interact based on the "collision bounding box".
///        This version is for character-particle collisions.
bool test_interaction_close_2( const oct_bb_t * cv_a, const fvec3_t& pos_a, const oct_bb_t * cv_b, const fvec3_t& pos_b, int test_platform );

/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-particle collisions.
bool get_depth_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth );
/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-particle collisions.
bool get_depth_1( const oct_bb_t * cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth );
/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-character collisions
bool get_depth_2( const oct_bb_t * cv_a, const fvec3_t& pos_a, const oct_bb_t * cv_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth );
/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-particle collisions.
bool get_depth_close_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth );
/// @brief Estimate the depth of collision based on the "collision bounding box".
///        This version is for character-particle collisions.
bool get_depth_close_1( const oct_bb_t * cv_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth );
/// @brief Estimate the depth of collision based on the "collision bounding box"
///        This version is for character-character collisions.
bool get_depth_close_2( const oct_bb_t * cv_a, const oct_bb_t * cv_b, bool break_out, oct_vec_t depth );
