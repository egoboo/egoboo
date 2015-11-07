#pragma once

// This is the definition of FIXNUM from CARTMAN
#define FIXNUM    4 // 4.129 // 4.150
// This is the definition of CARTMAN_FIXNUM from EGOLIB
#define CARTMAN_FIXNUM 4.125f ///< 4.150f        ///< Magic number
#define CARTMAN_SLOPE 50                        ///< increments for terrain slope (related to twist?).

constexpr uint16_t TILE_UPPER_SHIFT = 8;
constexpr uint16_t TILE_LOWER_MASK = ((1 << TILE_UPPER_SHIFT) - 1);
constexpr uint16_t TILE_UPPER_MASK = (~TILE_LOWER_MASK);

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )

// tile (aka fan) constants
#define TWIST_FLAT                119
#define MAP_FAN_VERTICES_MAX      16                         ///< Fansquare vertices
#define MAP_FAN_TYPE_MAX          64                         ///< Number of fansquare command types
#define MAP_FAN_MAX               4                          ///< Draw up to 4 fans
#define MAP_FAN_ENTRIES_MAX       32                         ///< Fansquare command list size
#define MAP_FAN_SIZE_MAX          32                         ///< Max trigs in each command

/**
 * @brief
 *  If cartman_map_tile_t::tx_bits or ego_tile_info_t::_img is this value, then do not draw the tile.
 */
#define MAP_FANOFF 0xFFFF                     ///< Don't draw the fansquare if tile = this

/// The bit flags for mesh tiles
enum e_map_fx : uint8_t
{
	MAPFX_REF = 0,     ///< NOT USED
					   ///< Egoboo v1.0 : "0 This tile is drawn 1st"

	MAPFX_SHA = (1 << 0),  ///< 0 == (val & MAPFX_SHA) means that the tile is reflected in the floors
						   ///< Egoboo v1.0: "0 This tile is drawn 2nd"
						   ///< aicodes.txt : FXNOREFLECT

	MAPFX_REFLECTIVE = (1 << 1),  ///< the tile reflects entities
								  ///< Egoboo v1.0: "1 Draw reflection of characters"
								  ///< aicodes.txt : FXDRAWREFLECT

	MAPFX_ANIM = (1 << 2),    ///< Egoboo v1.0: "2 Animated tile ( 4 frame )"
							  ///< aicodes.txt : FXANIM

	MAPFX_WATER = (1 << 3),   ///< Egoboo v1.0: "3 Render water above surface ( Water details are set per module )"
							  ///< aicodes.txt : FXWATER

	MAPFX_WALL = (1 << 4),    ///< Egoboo v1.0: "4 Wall ( Passable by ghosts, particles )"
							  ///< aicodes.txt : FXBARRIER

	MAPFX_IMPASS = (1 << 5),  ///< Egoboo v1.0: "5 Impassable"
							  ///< aicodes.txt : FXIMPASS

	MAPFX_DAMAGE = (1 << 6),  ///< Egoboo v1.0: "6 Damage"
							  ///< aicodes.txt : FXDAMAGE

	MAPFX_SLIPPY = (1 << 7)   ///< Egoboo v1.0: "7 Ice or normal"
							  ///< aicodes.txt : FXSLIPPY
};