


#pragma once

#include "egolib/bbox.h"
#include "egolib/bsp_aabb.h"

namespace BSP
{
	template <typename Type>
	struct Bounds {
	};
	/**
	 * @brief
	 *	A convex bounding volume which can be empty i.e. contains no points.
	 * @todo
	 *	Make the actual type of the bounding volume a template argument.
	 */
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
	};
#if 0
	template <>
	struct Bounds <BSP_aabb_t>
	{
	private:
		bool _empty;
		BSP_aabb_t _volume;
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
			}
			else {
				bv_self_union(&_volume, &volume);
			}
			_empty = false;
		}
		void set(const BSP_aabb_t& volume)
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
#endif

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
			}
			else {
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
};