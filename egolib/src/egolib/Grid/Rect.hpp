#pragma once

#include "egolib/Grid/Index.hpp"

namespace Grid {
template <typename _Type, CoordinateSystem _CoordinateSystem>
struct Rect {

	Index<_Type, _CoordinateSystem> _min;

	Index<_Type, _CoordinateSystem> _max;

	Rect(const Index<_Type, _CoordinateSystem>& min, const Index<_Type, _CoordinateSystem>& max)
		: _min(min), _max(max) {}

	Rect(const Rect& other)
		: _min(other._min), _max(other._max) {}
};
}