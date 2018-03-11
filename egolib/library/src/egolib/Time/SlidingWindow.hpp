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

/// @file egolib/Time/SlidingWindow.hpp
/// @brief A sliding window
/// @author Michael Heilmann

#pragma once

#include "egolib/typedef.h"

namespace Ego {
namespace Time {

/**
 * @remark
 *	A sliding window over an infinite stream of data points. Useful for implementing online algorithms.
 * @remark
 *	A sliding window over an infinite stream of data points is a fixed-capacity circular buffer.
 *	Data points are stored in the window in the ordered in which they are received from the stream.
 *	If the window is full then the oldest data point is dropped.
 * @remark
 *	Sliding 
 * @tparam _DataPointType
 *  the type of the data points of this sliding window
 * @author
 *	Michael Heilmann
 */
template <typename _DataPointType>
struct SlidingWindow : private idlib::non_copyable {

	/**
	 * @brief
	 *	The type of the data points of this sliding point.
	 */
    using DataPointType = _DataPointType;

private:

	/**
     * @brief
     *  The position of the oldest element.
     * @invariant
     *  <tt>_old < _capacity</tt>
     */
    size_t _old;

    /**
     * @brief
     *  The position at which to insert a new element.
     * @invariant
     *  <tt>_write < _capacity</tt>
     */
    size_t _new;

	/**
     * @brief
     *  The size of this circular buffer.
     * @invariant
     *  <tt>_size <= _capacity</<tt>
     */
    size_t _size,

    /** 
     * @brief
     *  The capacity of this circular buffer.
     * @invariant
     *  <tt>_capacity > 0</tt>
     */
           _capacity;
    /**
     * @brief
     *  The backing array.
     * @invariant
     *  <tt>nullptr != _elements</tt>
     *  <tt>|_elements| == _capacity</tt>
     */
	_DataPointType *_dataPoints;

protected:
	/**
	 * @brief
	 *	Invoked if a data point was removed.
	 * @param dataPoint
	 *	the data point
	 */
#if defined(_MSC_VER) // Disable sickening flood of warnings.
    #pragma warning(push)
    #pragma warning(disable: 4100)
#endif
	virtual void onRemove(const DataPointType& dataPoint) { }
#if defined(_MSC_VER)
    #pragma warning(pop)
#endif
	/**
	 * @brief
	 *	Invoked if a data point was added.
	 * @param dataPoint
	 *	the data point
	 */
#if defined(_MSC_VER) // Disable sickening flood of warnings.
    #pragma warning(push)
    #pragma warning(disable: 4100)
#endif
	virtual void onAdd(const DataPointType& dataPoint) { }
#if defined(_MSC_VER)
    #pragma warning(pop)
#endif
	/**
	 * @brief
	 *	Invoked if all data points were removed.
	 */
	virtual void onClear() { }
public:
    /**
     * @brief
     *  Construct this sliding window.
     * @param capacity
     *  the initial capacity of this sliding window
     * @throw std::invalid_argument
     *  if @a capacity is @a 0
	 * @remark
	 *	\f$old = new = size = 0\f$. As \f$capacity > 0\f$ holds, it follows that \f$old, new, size < capacity\f$.
     */
	SlidingWindow(size_t capacity)
		: _old(0), _new(0), _size(0), _capacity(capacity), _dataPoints(new DataPointType[capacity]) {
        if (0 == capacity) {
			delete[] _dataPoints;
			_dataPoints = nullptr;
            throw std::invalid_argument("capacity is 0");
        }
    }
    /** 
     * @brief
     *  Destruct this sliding window.
     */
	virtual ~SlidingWindow() {
		delete[] _dataPoints;
		_dataPoints = nullptr;
    }
    /** 
     * @brief
     *  Get the size of this sliding window.
     * @return
     *  the size of this sliding window
     */
    size_t size() const {
        return _size;
    }
    /**
     * @brief
     *  Get the capacity of this sliding window.
     * @return
     *  the capacity of this sliding window
     */
    size_t capacity() const {
        return _capacity;
    }
    /**
     * @brief
     *  Get if this sliding window is full.
     * @return
     *  @a true if this sliding window is full,
     *  @a false otherwise
     */
    bool full() const {
        return _capacity == _size;
    }
    /**
     * @brief
     *  Get if this sliding window is empty.
     * @return
     *  @a true if this sliding window is empty,
     *  @a false otherwise
     */
    bool empty() const {
        return 0 == _size;
    }

    /**
     * @brief
     *  Clear this sliding window.
	 * @remark
	 *	\f$old = new = size = 0\f$. As \f$capacity > 0\f$ holds, it follows that \f$old, new, size < capacity\f$.
     */
    void clear() {
        _old = 0; 
        _new = 0;
        _size = 0;
		onClear();
    }

	/**
	 * @brief
	 *	Get the data point at the specified index.
	 * @param index
	 *	the index
	 * @return
	 *	the data point
	 * @throw std::invalid_argument
	 *	if the index is greater than or equal to the size
	 */
	const _DataPointType& get(size_t index) const {
		if (index >= _size) {
			throw std::invalid_argument("index out of bounds");
		}
		return _dataPoints[(_old + index) % _capacity];

	}

    /**
     * @brief
     *  Add a data point to this sliding window.
     * @param value
     *  the value
     */
	void add(const _DataPointType& dataPoint) {
		if (!empty() && _new == _old) {
			onRemove(_dataPoints[_old]);
			_old = (_old + 1) % _capacity;
		}
		_dataPoints[_new] = dataPoint;
		_new = (_new + 1) % _capacity;
		_size = (_size + 1) % _capacity;
		onAdd(dataPoint);
    }
    
};

} // namespace Time
} // namespace Ego
