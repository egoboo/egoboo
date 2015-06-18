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

/// @file  egolib/bsp_aabb.h
/// @brief BSP AABBs

#include "egolib/bsp_aabb.h"
#include "egolib/bbox.h"

#if 0
struct BSP_vec_t
{

private:

	size_t _d;

	float *_vS;

public:

	BSP_vec_t(size_t d)
		: _d(d),
		  _vS(new float[d]())
	{
	}
	~BSP_vec_t()
	{
		delete[] _vS;
		_vS = nullptr;
	}

	void throwOutOfRange(const char *file, int line,size_t i) const
	{
		std::ostringstream s;
		s << __FILE__ << ": " << __LINE__ << ": ";
		s << "index " << i << " out of bounds [" << 0 << ", " << _d << "]";
		throw std::out_of_range(s.str());
	}

public:

	bool operator==(const BSP_vec_t& other) const
	{
		if (_d != other._d) return false;
		for (size_t i = 0, n = _d; i < n; ++i)
		{
			if (_vS[i] != other._vS[i]) return false;
		}
		return true;
	}

	bool operator!=(const BSP_vec_t& other) const
	{
		if (_d != other._d) return true;
		for (size_t i = 0, n = _d; i < n; ++i)
		{
			if (_vS[i] != other._vS[i]) return true;
		}
		return false;
	}

	const float& operator[](size_t i) const
	{
		if (i >= _d) throwOutOfRange(__FILE__, __LINE__,i);
		return _vS[i];
	}

	float& operator[](size_t i)
	{
		if (i >= _d) throwOutOfRange(__FILE__, __LINE__,i);
		return _vS[i];
	}

};
#endif

BSP_aabb_t::BSP_aabb_t(size_t dim) :
	_empty(false), 
	_values(new float[3*dim]()), 
	_dim(dim)
{
}

//--------------------------------------------------------------------------------------------
BSP_aabb_t::~BSP_aabb_t()
{
	delete[] _values;
	_values = nullptr;
}

//--------------------------------------------------------------------------------------------
void BSP_aabb_t::setDim(size_t newDim,bool preserve)
{
	if (_dim == newDim)
	{
		return;
	}
	float *newValues = new float[3 * newDim]();
	// An alias to the old dimensionality.
	size_t oldDim = _dim;
    if (preserve)
    {
        // The smaller of the old and the new dimensionality.
        size_t minDim = std::min(oldDim, newDim);
        // Copy values of existing dimensions.
        for (size_t i = 0, n = 3; i < n; ++i)
        {
            size_t dstOffset = i * newDim;
            size_t srcOffset = i * oldDim;
            for (size_t j = 0, m = minDim; j < m; ++j)
            {
                newValues[dstOffset + j] = _values[srcOffset + j];
            }
        }
    }
	delete[] _values;
	_values = newValues;
	_dim = newDim;
}

void BSP_aabb_t::set(const oct_bb_t& other)
{
	if (!_dim) return;

	// this process is a little bit complicated because the
	// order to the OCT_* indices is optimized for a different test.
	switch (_dim)
	{
	case 1:
	{
		min()[kX] = other._mins[OCT_X];
		max()[kX] = other._maxs[OCT_X];
	}
	case 2:
	{
		min()[kX] = other._mins[OCT_X];
		min()[kY] = other._mins[OCT_Y];

		max()[kX] = other._maxs[OCT_X];
		max()[kY] = other._maxs[OCT_Y];
	}
	default:
	{
		min()[kX] = other._mins[OCT_X];
		min()[kY] = other._mins[OCT_Y];
		min()[kZ] = other._mins[OCT_Z];

		max()[kX] = other._maxs[OCT_X];
		max()[kY] = other._maxs[OCT_Y];
		max()[kZ] = other._maxs[OCT_Z];

		// Blank any extended dimensions.
		for (size_t i = 3, n = _dim; i < n; ++i)
		{
			min()[i] = max()[i] = 0.0f;
		}
	}
	};

	// Recompute mid values.
	for (size_t i = 0, n = _dim; i < n; ++i)
	{
		mid()[i] = 0.5f * (min()[i] + max()[i]);
	}
	_empty = false;
}

void BSP_aabb_t::set(const BSP_aabb_t& other)
{
	if (other._empty)
	{
		_empty = true;
	}
	else
	{
		// Ensure that they have the same dimensions.
		setDim(other._dim,false);
		for (size_t i = 0; i < _dim; i++)
		{
			min()[i] = other.min()[i];
			mid()[i] = other.mid()[i];
			max()[i] = other.max()[i];
		}
		_empty = false;
	}
}

void BSP_aabb_t::add(const BSP_aabb_t& other)
{
	if (other._empty)
	{
		return;
	}
	else if (_empty)
	{
		set(other);
	}
	else
	{
		size_t min_dim = std::min(_dim, other._dim);
		for (size_t i = 0; i < min_dim; i++)
		{
			min()[i] = std::min(min()[i], other.min()[i]);
			max()[i] = std::max(max()[i], other.max()[i]);
			mid()[i] = 0.5f *  (min()[i] + max()[i]);
		}
	}
}

bool BSP_aabb_t::empty() const
{
	return _empty;
}

void BSP_aabb_t::clear()
{
	_empty = true;
}

bool BSP_aabb_t::overlaps(const BSP_aabb_t& other) const
{
	if (_empty || other._empty)
	{
		return false;
	}
	size_t min_dim = std::min(_dim, other._dim);
	if (0 == min_dim)
	{
		return false;
	}
	const float *other_mins = other.min();
	const float *other_maxs = other.max();
	const float *this_mins  = min();
	const float *this_maxs  = max();
	for (size_t i = 0; i < min_dim; i++, this_mins++, this_maxs++, other_mins++, other_maxs++)
	{
		if ((*other_maxs) < (*this_mins)) return false;
		if ((*other_mins) > (*this_maxs)) return false;
	}
	return true;
}

bool BSP_aabb_t::contains(const BSP_aabb_t& other) const
{
	if (_empty || other._empty) return false;

	size_t min_dim = std::min(_dim, other._dim);
	if (0 == min_dim) return false;

	// The optimizer is supposed to do this stuff all by itself, but isn't.
	const float *other_mins = other.min();
	const float *other_maxs = other.max();
	const float *this_mins = min();
	const float *this_maxs = max();

	for (size_t i = 0; i < min_dim; i++, other_mins++, other_maxs++, this_mins++, this_maxs++)
	{
		if ((*other_maxs) > (*this_maxs)) return false;
		if ((*other_mins) < (*this_mins)) return false;
	}

	return true;
}

bool BSP_aabb_t::overlaps(const aabb_t& other) const
{
	if (_empty) return false;

	size_t min_dim = std::min(_dim,(size_t)3);
	if (0 == min_dim) return false;

	for (size_t i = 0; i < min_dim; i++)
	{
		if ((other.getMax()[i]) < (min()[i])) return false;
		if ((other.getMin()[i]) > (max()[i])) return false;
	}

	return true;
}

bool BSP_aabb_t::contains(const aabb_t& other) const
{
	if (_empty) return false;

	size_t min_dim = std::min(_dim,(size_t)3);
	if (0 == min_dim) return false;


	for (size_t i = 0; i < min_dim; i++)
	{
		if (other.getMax()[i] > max()[i]) return false;
		if (other.getMin()[i] < min()[i]) return false;
	}

	return true;
}

const float *BSP_aabb_t::min() const
{
	return _values + 0 * _dim;
}

const float *BSP_aabb_t::mid() const
{
	return _values + 1 * _dim;
}

const float *BSP_aabb_t::max() const
{
	return _values + 2 * _dim;
}

float *BSP_aabb_t::min()
{
	return _values + 0 * _dim;
}

float *BSP_aabb_t::mid()
{
	return _values + 1 * _dim;
}

float *BSP_aabb_t::max()
{
	return _values + 2 * _dim;
}
