#pragma once

#include "game/egoboo_typedef.h"

namespace Grid {
/**
* @brief
*  An enumeration of coordinate system used by/for/together with meshes.
*/
enum class CoordinateSystem {
	/**
	* @brief
	*  "tile grid"/"2D" coordinates.
	*/
	Grid,
	/**
	* @brief
	*  "tile list"/"1D" coordinates.
	*/
	List,
};
}

namespace  Grid {
/**
 * @brief
 *  An index "tile grid"/"2D" or "tile list"/"1D" coordinates.
 */
template <typename _Type, CoordinateSystem _CoordinateSystem, typename _Enabled = void>
struct Index;
}


namespace Grid {
/**
 * @brief
 *  A "tile grid"/"2D" index.
 */
template <typename _Type, CoordinateSystem _CoordinateSystem>
struct Index<_Type, _CoordinateSystem,
	         typename std::enable_if<(_CoordinateSystem == CoordinateSystem::Grid) && std::is_same<_Type, int>::value>::type
            >
{
private:
	_Type _x; ///< @brief The x-coordinate.
	_Type _y; ///< @brief The y-coordinate.

public:
	typedef Index<_Type, _CoordinateSystem> MyType;

public:
	Index(const _Type& x, const _Type& y) :
		_x(x), _y(y)
	{}

public:
	MyType& operator=(const MyType& other) {
		_x = other._x;
		_y = other._y;
		return *this;
	}

public:
	bool operator==(const MyType& other) const {
		return _x == other._x
			&& _y == other._y;
	}

	bool operator!=(const MyType& other) const {
		return _x != other._x
			|| _y != other._y;
	}

public:
	const _Type& getX() const {
		return _x;
	}

	const _Type& getY() const {
		return _y;
	}

};
}

namespace Grid {
/**
 * @brief
 *  A "list"/"1D" index.
 */
template <typename _Type, CoordinateSystem _CoordinateSystem>
struct Index<_Type, _CoordinateSystem,
	         typename std::enable_if<(_CoordinateSystem == CoordinateSystem::List) && std::is_same<_Type, int>::value>::type
            >
{
private:

	_Type _i; ///< @brief The i-coordinate.

	static const _Type _InvalidIndex = std::numeric_limits<_Type>::max();

public:
	typedef Index<_Type, _CoordinateSystem> MyType;

	static const MyType Invalid;

	Index() :
		_i(_InvalidIndex)
	{}

	Index(const _Type& i) :
		_i(i)
	{}

public:

	Index(const MyType& other) :
		_i(other._i)
	{}

	MyType& operator=(const MyType& other)
	{
		_i = other._i;
		return *this;
	}

public:

	bool operator==(const MyType& other) const {
		return _i == other._i;
	}

	bool operator!=(const MyType& other) const {
		return _i != other._i;
	}

public:

	bool operator<(const MyType& other) const {
		return _i < other._i;
	}

	bool operator<=(const MyType& other) const {
		return _i <= other._i;
	}

	bool operator>(const MyType& other) const {
		return _i > other._i;
	}

	bool operator>=(const MyType& other) const {
		return _i >= other._i;
	}

public:

	const _Type& getI() const {
		return _i;
	}

public:

	MyType& operator++() {
		_i++;
		return *this;
	}

	MyType operator++(int) {
		_Type j = _i;
		_i++;
		return MyType(j);
	}

};

template <typename _Type, CoordinateSystem _CoordinateSystem>
const Index<_Type, _CoordinateSystem> Index<_Type, _CoordinateSystem,
	        typename std::enable_if<(_CoordinateSystem == CoordinateSystem::List) && std::is_same<_Type, int>::value>::type
           >::Invalid;
}


#if 0
Index<int, CoordinateSystem::List> toList(Index<int, CoordinateSystem::Grid> source, int width) {
	// Assume a grid of 3 x 3 elements as displayed below:
	// 0  1  2
	// 3  4  5
	// 6  7  8
	// 9 10 11
	// The grid index row y = 3 and the column x = 1 should map to list index i = 10.
	// Using the formula we obtain i = 3 * 3 + 1 = 10 as desired.
	return Index<int, CoordinateSystem::List>(source.getX() + source.getY() * width);
}

Index<int, CoordinateSystem::Grid> toGrid(Index<int, CoordinateSystem::List> source, int width) {
	// Assume a grid of 3 x 3 elements as displayed below:
	// 0  1  2
	// 3  4  5
	// 6  7  8
	// 9 10 11
	// The index i = 10 should map to row y = 3 and to column x = 1.
	// Using the formula we obtain x = 10 % 3 = 1 and y = 10 / 3 = 3 as desired.
	return Index<uint32_t, CoordinateSystem::Grid>(source.getI() % width, source.getI() / width);
}
#endif