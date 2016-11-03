#pragma once

#include "egolib/platform.h"

namespace Ego
{
/// @a (TileFX) determines properties of a tile.
/// @a (TileFX) are default-constructible, copy-constructible, assignable.
/// In particular, they support the operators
/// @code{|},
/// @code{&},
/// @code{|=},
/// @code{&=},
/// @code{~},
/// @code{=},
/// @code{==},
/// @code{!=}.
struct TileFX : public Id::Equatable<TileFX>
{
private:
    uint8_t fx;

private:
    explicit TileFX(uint8_t fx);

public:
    TileFX();

    TileFX(TileFX&& rhs);

    TileFX(const TileFX& rhs);


public:
	TileFX& operator=(const TileFX& other);

	TileFX& operator=(TileFX&& other);

public:
	// CRTP
    bool equalTo(const TileFX& other) const noexcept;
	
public:
    /// @warning do not return by reference to no-const.
    /// If a reference to non-const was returned, then
    /// an expression @code{x = ~y} where @code{x} and
    /// @code{y} are variables, the value of @code{y}
    /// would change.
	TileFX operator~() const;

	/// Implemented in terms of |=.
	/// @see http://en.cppreference.com/w/cpp/language/operators
    friend TileFX operator|(TileFX lhs, const TileFX& rhs);

	/// Implemented in terms of |=.
	/// @see http://en.cppreference.com/w/cpp/language/operators
	friend TileFX operator&(TileFX lhs, const TileFX& rhs);

public:
	TileFX& operator&=(const TileFX& rhs);

    TileFX& operator|=(const TileFX& rhs);

public:
    /// Is the tile an animated tile?
    bool isAnimated() const;
    
    /// Is the tile a water tile?
    bool isWater() const;

    /// Is the tile a slippery tile?
    bool isSlippery() const;

    /// Is the tile a reflective tile?
    bool isReflective() const;
    
    /// Is the tile a passable tile?
    /// impassable tiles can not be passed by any entity.
    bool isImpassable() const;
    
    /// Is the tile a wall tile?
    /// wall tiles can not be passed by certain entities (e.g. horse) and can be passed by other entities (e.g. ghosts).
    bool isWall() const;
    
    /// Is the tile a damaging tile?
    bool isDamaging() const;

    /// Is the tile reflected?
    bool isReflected() const;

public:
	/// A tile with no properties.
	static const TileFX None;

	/// A tile with all properties.
	static const TileFX All;

    /// Property of being a "water" tile.
    static const TileFX Water;

    /// Property of being a "slippery" tile.
    /// Propety of a surface, having low friction, often due to being covered in a non - viscous liquid,
    /// and therefore hard to grip, hard to stand on without falling, etc. "the frozen surface of a
    /// lake" or a  "stone floor covered by ice or oil" are examples of slippery surfaces.
    static const TileFX Slippery;
    
    /// Property of being a "damaging" tile.
    /// Property of causing damage to entities (e.g. humans) on or in proximity of the tile.
    static const TileFX Damaging;
    
    /// Property of being an "animated" tile.
    static const TileFX Animated;
    
    /// Property of being a "reflective" tile.
    /// Reflective (!= reflected) tiles are reflecting entities (e.g. objects and particles).
    static const TileFX Reflective;

    /// Property of being a "reflected" tile.
    /// Reflected (!= reflective) tiles are reflected by non-reflecting tiles.
    static const TileFX Reflected;

    /// Property of being a "wall" tile.
    /// Wall tiles can not be passed by some entities (e.g. a human can - usually - not pass a wall)
    /// However, certain entities can pass walls (e.g. ghosts).
    static const TileFX Wall;

    /// Property of being an "impassable" tile.
    /// No entity (e.g. objets and particles) can pass an impassable tile.
    static const TileFX Impassable;
};

} // namespace Ego