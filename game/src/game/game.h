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

/// @file game/game.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/mesh.h"
#include "game/input.h"
#include "game/Inventory.hpp"
#include "game/Shop.hpp"

//--------------------------------------------------------------------------------------------
// forward declaration of external structs
//--------------------------------------------------------------------------------------------

struct prt_bundle_t;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EXPKEEP 0.85f                                ///< Experience to keep when respawning

// tile
// #defne TILESOUNDTIME 16
#define TILE_REAFFIRM_AND  3

// status list
#define MAX_STATUS          10                             ///< Maximum status displays

// end text
#define MAXENDTEXT 1024                                    ///< longest end text message

// imports
#define MAX_IMPORTS 16
#define MAX_IMPORT_OBJECTS     ( Inventory::MAXNUMINPACK + 2 )        ///< left hand + right hand + MAXINVENTORY
#define MAX_IMPORT_PER_PLAYER  ( 1 + MAX_IMPORT_OBJECTS )  ///< player + MAX_IMPORT_OBJECTS

//--------------------------------------------------------------------------------------------

/// The offsets of Bits identifying in-game actions in a Bit set.
enum e_latchbutton_bits
{
    LATCHBUTTON_LEFT      = 0,                      ///< Character button presses
    LATCHBUTTON_RIGHT     = 1,
    LATCHBUTTON_JUMP      = 2,
    LATCHBUTTON_ALTLEFT   = 3,                      ///< ( Alts are for grab/drop )
    LATCHBUTTON_ALTRIGHT  = 4,
    LATCHBUTTON_PACKLEFT  = 5,                      ///< ( Used by AI script for inventory cycle )
    LATCHBUTTON_PACKRIGHT = 6,                      ///< ( Used by AI script for inventory cycle )
    LATCHBUTTON_RESPAWN   = 7,
};

//--------------------------------------------------------------------------------------------

/// The possible pre-defined orders
enum e_order
{
    ORDER_NONE  = 0,
    ORDER_ATTACK,
    ORDER_ASSIST,
    ORDER_STAND,
    ORDER_TERRAIN,
    ORDER_COUNT
};

//--------------------------------------------------------------------------------------------

/// The bitmasks used by the chr_check_target() function which is used in various character search
/// functions like chr_find_target() or find_object_in_passage()
enum e_targeting_bits
{
    TARGET_DEAD         = ( 1 << 0 ),       ///< Target dead stuff
    TARGET_ENEMIES      = ( 1 << 1 ),       ///< Target enemies
    TARGET_FRIENDS      = ( 1 << 2 ),       ///< Target friends
    TARGET_ITEMS        = ( 1 << 3 ),       ///< Target items
    TARGET_INVERTID     = ( 1 << 4 ),       ///< Target everything but the specified IDSZ
    TARGET_PLAYERS      = ( 1 << 5 ),       ///< Target only players
    TARGET_SKILL        = ( 1 << 6 ),       ///< Target needs the specified skill IDSZ
    TARGET_QUEST        = ( 1 << 7 ),       ///< Target needs the specified quest IDSZ
    TARGET_SELF         = ( 1 << 8 )        ///< Allow self as a target?
};

//--------------------------------------------------------------------------------------------

/// The state of the animated tiles.
struct AnimatedTilesState {
    /// The state of a layer of the animated tiles.
    struct Layer {
        Layer() :
            update_and(0),
            frame_and(0),
            base_and(0),
            frame_add(0),
            frame_add_old(0),
            frame_update_old(0)
        {
            //ctor
        }

        int    update_and;            ///< how often to update the tile

        uint16_t frame_and;             ///< how many images within the "tile set"?
        uint16_t base_and;              ///< animated "tile set"
        uint16_t frame_add;             ///< which image within the tile set?
        uint16_t frame_add_old;         ///< the frame offset, the last time it was updated
        uint32_t frame_update_old;
    };
    std::array<Layer,2> elements;
    void upload(const wawalite_animtile_t& source);
    /// @brief Iterate the state of the animated tiles.
    void animate();
};


//--------------------------------------------------------------------------------------------

int update_game();

//--------------------------------------------------------------------------------------------

/// The state of the weather.
struct WeatherState
{
    int timer_reset;                    ///< How long between each spawn?
    bool  over_water;                   ///< Only spawn over water?
    LocalParticleProfileRef part_gpip;  ///< Which particle to spawn?

    PLA_REF iplayer;
    int     time;                       ///< 0 is no weather

	void upload(const wawalite_weather_t& source);
    /// @brief Iterate the state of the weather.
    /// @remarks Drops snowflakes or rain or whatever.
    void animate();
};


//--------------------------------------------------------------------------------------------

/// The in-game fog state
/// @warn Fog is currently not used
struct fog_instance_t
{
    bool _on;            ///< Do ground fog?
    float _top,
		  _bottom;
    Uint8 _red, _grn, _blu;
    float _distance;

	void upload(const wawalite_fog_t& source);
};

//--------------------------------------------------------------------------------------------
// Imports
struct import_element_t
{
    STRING          srcDir;
    STRING          dstDir;
    STRING          name;

    size_t          local_player_num;   ///< Local player number (player 1, 2, 3 or 4)
    size_t          player;             ///< Which player is this?
    int             slot;               ///< which slot it it to be loaded into

	import_element_t() {
		srcDir[0] = '\0';
		dstDir[0] = '\0';
		name[0] = '\0';
		local_player_num = 0;
		// all non-zero, non-null values
		player = INVALID_PLA_REF;
		slot = -1;
	}

	static void init(import_element_t& self);
};



//--------------------------------------------------------------------------------------------

struct import_list_t
{
    size_t           count;              ///< Number of imports
    import_element_t lst[MAX_IMPORTS];

	import_list_t()
		: count(0), lst() {
	}

	static void init(import_list_t& self);
	static egolib_rv from_players(import_list_t& self);
};

//--------------------------------------------------------------------------------------------

/// List of objects with status displays
struct status_list_t
{
	/// Status display info
	struct element_t
	{
		int camera_index;
		ObjectRef who;
		element_t()
			: camera_index(-1), who(ObjectRef::Invalid) {
		}
	};
    bool on;
    size_t count;
    element_t lst[MAX_STATUS];

	status_list_t()
		: on(false), count(0), lst() {
	}
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// special terrain and wawalite-related data structs (TODO: move into Module class)
extern WeatherState g_weatherState;
extern fog_instance_t fog;
extern AnimatedTilesState g_animatedTilesState;

// End text
extern char   endtext[MAXENDTEXT];     ///< The end-module text
extern size_t endtext_carat;

extern bool    overrideslots;          ///< Override existing slots?
extern FACING_T  glouseangle;            ///< global return value from prt_find_target() - actually still used

extern import_list_t g_importList;

// various clocks and timers
extern Uint32          clock_enc_stat;        ///< For character stat regeneration
extern Uint32          clock_chr_stat;        ///< For enchant stat regeneration
extern Uint32          update_wld;            ///< The number of times the game has been updated

// counters for debugging wall collisions
extern int chr_stoppedby_tests;
extern int chr_pressure_tests;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the hook for deinitializing an old module
void game_quit_module();

/// the hook for exporting all the current players and reloading them
bool game_finish_module();
bool game_begin_module(const std::shared_ptr<ModuleProfile> &module);
void game_load_module_profiles(const std::string& modname);
void game_load_global_profiles();

/// Exporting stuff
egolib_rv export_one_character( ObjectRef character, ObjectRef owner, int chr_obj_index, bool is_local );
egolib_rv export_all_players( bool require_local );

// save character functions
bool  export_one_character_quest_vfs( const char *szSaveName, ObjectRef character );
bool  export_one_character_name_vfs( const char *szSaveName, ObjectRef character );

/// Messages
void show_armor( int statindex );
void show_full_status( int statindex );
void show_magic_status( int statindex );

/// End Text
void reset_end_text();

/// Particles
/// @brief Get the number of particles attached to an object.
int number_of_attached_particles(ObjectRef objectRef);
/// @brief Make sure an object has no particles attached
/// @return the number of particles removed
void    disaffirm_attached_particles(ObjectRef objectRef);
/// @brief Make sure an object has all particles attached
/// @return the number of particles added
int reaffirm_attached_particles(ObjectRef objectRef);

//Latches
bool chr_do_latch_button( Object * pchr );
bool chr_do_latch_attack( Object * pchr, slot_t which_slot );
void character_swipe( ObjectRef cnt, slot_t slot );

/// AI targeting
bool  chr_check_target( Object * psrc, const std::shared_ptr<Object>& ptst, const IDSZ2& idsz, const BIT_FIELD targeting_bits );
ObjectRef chr_find_target( Object * psrc, float max_dist, const IDSZ2& idsz, const BIT_FIELD targeting_bits );
ObjectRef prt_find_target( const Vector3f& pos, FACING_T facing, const PIP_REF ipip, const TEAM_REF team, ObjectRef dontTarget, ObjectRef oldTarget, FACING_T *targetAngle);

void expand_escape_codes( const ObjectRef ichr, script_state_t * pstate, char * src, char * src_end, char * dst, char * dst_end );

Uint8 get_alpha( int alpha, float seeinvis_mag );
Uint8 get_light( int alpha, float seedark_mag );

bool attach_one_particle( prt_bundle_t * pbdl_prt );

ObjectRef chr_get_lowest_attachment(ObjectRef object_ref, bool non_item );

egolib_rv game_copy_imports( import_list_t * imp_lst );

//--------------------------------------------------------------------------------------------
/**
 * Zeitgeist connects the game and the real world including. Functionality like audio
 * and video communication and social networking support might be integrated here.
 */
namespace Zeitgeist {
// An enumeration of special times.
enum class Time {
    Halloween,       // Halloween.
    Christmas,       // Christmas.
    Nighttime,       // Nighttime.
    Daytime,         // Daytime.
};
/// @brief Get if the specified time is.
/// @param time the time
/// @return @a true if
bool CheckTime(Time time);
}
//--------------------------------------------------------------------------------------------

void   game_update_timers();

// wawalite functions
struct wawalite_data_t * read_wawalite_vfs();
bool write_wawalite_vfs( const wawalite_data_t * pdata );
bool wawalite_finalize( wawalite_data_t * pdata );
void upload_wawalite();

// Mesh query.
float get_chr_level(ego_mesh_t *mesh, Object *object);
// Mesh query.
float get_mesh_max_vertex_1(ego_mesh_t *mesh, const Index2D& index2D, oct_bb_t& bump, bool waterwalk);
// Mesh query.
float get_mesh_max_vertex_2(ego_mesh_t *mesh, Object *object);

void playMainMenuSong();