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

#pragma once

#include "egolib/bbox.h"
#include "egolib/bsp_aabb.h"

namespace BSP
{
	/**
	* @brief
	*	Template to add support for being "empty" to an arbitrary bounding volume.
	* @remark
	*	A bounding volume (implicitly) describe a set of points it contains. Typical
	*	bounding volume implementations in Egoboo don't have a concept of being empty
	*	i.e. containing absolutely no point. For instance, an AABB always contains
	*	at leaest one point, same holds for spheres and other bounding volume
	*	primitives. By using template specialization, this class can tightly
	*	wrap around such a bounding volume and add the concept of being completely
	*	empty.
	*/
	template <typename Type>
	struct Bounds
	{
	};

	template <>
	struct Bounds<bv_t>
	{
	private:
		bool _empty;
		bv_t _volume;
	public:
		Bounds() :
			_empty(true),
			_volume()
		{
		}
		void clear()
		{
			_empty = true;
		}
		bool empty() const
		{
			return _empty;
		}
		const bv_t& get() const
		{
			if (_empty) throw std::domain_error("bounds empty");
			return _volume;
		}
		void add(const Bounds& bounds)
		{
			if (bounds._empty) return;
			else add(bounds._volume);
		}
		void add(const bv_t& volume)
		{
			if (_empty) {
				_volume = volume;
			} else {
				bv_self_union(&_volume, &volume);
			}
			_empty = false;
		}
		void set(const bv_t& volume)
		{
			_volume = volume;
			_empty = false;
		}
		geometry_rv intersects(const egolib_frustum_t& frustum) const
		{
			if (_empty) return geometry_outside;
			return frustum.intersects_bv(&_volume, true);
		}
		geometry_rv intersects(const aabb_t& aabb) const
		{
			if (_empty) return geometry_outside;
			return aabb_intersects_aabb(_volume.aabb, aabb);
		}
	};

	// Template specialization of Bounds for BSP_aabb_t.
	template <>
	struct Bounds<BSP_aabb_t>
	{
	private:
		bool _empty;
		BSP_aabb_t _volume;
	public:
		Bounds(size_t dim) :
			_empty(true),
			_volume(dim)
		{
		}
		void clear()
		{
			_empty = true;
		}
		bool empty() const
		{
			return _empty;
		}
		const BSP_aabb_t& get() const
		{
			if (_empty) throw std::domain_error("bounds empty");
			return _volume;
		}
		void add(const Bounds& bounds)
		{
			if (bounds._empty) return;
			else add(bounds._volume);
		}
		void add(const BSP_aabb_t& volume)
		{
			if (_empty) {
				_volume = volume;
			} else {
				_volume.add(volume);
			}
			_empty = false;
		}
		void set(const BSP_aabb_t& volume)
		{
			_volume.set(volume);
			_empty = false;
		}
#if 0
		geometry_rv intersects(const egolib_frustum_t& frustum) const
		{
			if (_empty) return geometry_outside;
			return frustum.intersects_bv(&_volume, true);
		}
		geometry_rv intersects(const aabb_t& aabb) const
		{
			if (_empty) return geometry_outside;
			return aabb_intersects_aabb(_volume.aabb, aabb);
		}
#endif
	};
};