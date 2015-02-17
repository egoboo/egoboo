#pragma once

#include "egolib/platform.h"

namespace Ego { namespace Script {

/**
 * @brief
 *	A dynamically resizing buffer for bytes.
 * @author
 *	Michael Heilmann
 */
class Buffer
{

private:

    /**
	 * @brief
	 *	The size of the buffer.
	 */
	size_t _size;

	/** 
	 * @brief
	 *	The capacity of the buffer.
	 */
	size_t _capacity;

    /**
     * @brief
     *  The elements of this buffer.
     */
    char *_elements;

public:

    /**
	 * @brief
	 *	Construct this buffer with the specified initial capacity.
	 * @param initialCapacity
	 *	the initial capacity of the buffer
	 * @throw std::bad_alloc
	 *	if not enough memory is available
	 */
	Buffer(size_t initialCapacity) :
		_size(0), _capacity(initialCapacity)
	{
		_elements = new char[initialCapacity];
	}

	/**
	 * @brief
	 *	Destruct this buffer.
	 */
	~Buffer()
	{
		delete[] _elements;
	}

    /**
     * @brief
     *  Get the size of this buffer.
     * @return
     *  the size
     */
    size_t getSize() const
    {
        return _size;
    }

    /**
     * @brief
     *  Get the capacity of this buffer.
     * @return
     *  the capacity
     */
    size_t getCapacity() const
    {
        return _capacity;
    }

    /**
     * @brief
     *  Get the maximum capacity of this buffer.
     * @return
     *  the maximum capacity
     */
    size_t getMaxCapacity() const
    {
        return std::numeric_limits<size_t>::max();
    }

    /**
     * @brief
     *  Increase the capacity by at least the specified additional required capacity.
     * @param req
     *  the additional capacity
     * @throw std::bad_array_new_length
     *  if the new capacity exceeds 
     * @throw std::bad_alloc
     *  if not enough memory is available
     */
    void increaseCapacity(size_t req)
    {
        if (0 == req) return;
        // This buffer is re-used and does not shrink.
        // Nevertheless we want to avoid frequent reallocations and if the requested additional capacity is too small,
        // add a bit more.
        size_t best;
        if (best < 5012)
        {
            best = 5012;
        }
        else
        {
            best = req;
        }
        // It is unlikely that the requested additional capacity ever exceeds
        // the maximum available additional capacity. However if it does ...
        if (getMaxCapacity() - best < _capacity)
        {
            // ... compute the available additional capacity.
            size_t avl = getMaxCapacity() - _capacity;
            // If the available additional capacity is smaller than then requested additional capacity,
            // raise an exception.
            if (avl < req)
            {
                // GCC 4.9 added std::bad_array_new_length() to libstdc++,
                // fallback on older versions.
                // This check doesn't support clang on libstdc++ though.
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 9))
                throw std::runtime_error("std::bad_array_new_length");
#else
                throw std::bad_array_new_length();
#endif
            }
            best = avl;
        }
        char *newElements = new char[_capacity + best];
        memcpy(newElements, _elements, _size);
        delete[] _elements;
        _elements = newElements;
        _capacity += best;
    }

    /**
     * @brief
     *  Clear this buffer.
     */
    void clear()
    {
        _size = 0;
    }

	/**
	 * @brief
	 *	Append a byte to the buffer growing the buffer if necessary.
	 * @param byte
	 *	the byte
	 * @throw std::bad_alloc
	 *	if not enough memory is available
	 */
	void append(char byte)
	{
		if (_size == _capacity)
		{
			increaseCapacity(1);
		}
		_elements[_size++] = byte;
	}
};

} }