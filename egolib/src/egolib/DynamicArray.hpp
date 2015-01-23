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

/// @file  egolib/DynamicAray.hpp
/// @brief generic dynamic array structure

#pragma once

#include "egolib/typedef.h"

namespace Ego
{
	template <typename ElementType> struct DynamicArray
	{
		int top;           ///< @brief The first @a size elements in the array pointed to by @a ary are valid,
		                   ///<        the remaining <tt>cp-size</tt> elements have unspecified values.
		                   ///< @todo      Rename to @a _size.
		                   ///< @todo      Change type to @a size_t.
		size_t cp;         ///< @brief     The number of elements in the array pointed to by @a ary.
		                   ///< @invariant Greater than @a 0.
		                   ///< @todo      Rename to @a _capacity.
		ElementType *ary;  ///< @brief A pointer to an array of @a cp @a ElementType elements.
		                   ///< @todo  Rename to @a _elements.

		/**
		 * @brief
		 *	Create a dynamic array.
		 * @param initialCapacity
		 *	the initial capacity of the array
		 * @return
		 *	a pointer to the array on success, @a NULL on failure
		 */
		DynamicArray<ElementType> *create(size_t initialCapacity)
		{
			DynamicArray<ElementType> *self;
			self = EGOBOO_NEW(DynamicArray<ElementType>);
			if (!self)
			{
				return NULL;
			}
			if (!self->ctor(initialCapacity))
			{
				EGOBOO_DELETE(self);
				return NULL;
			}
			return self;
		}

		/**
		 * @brief
		 *	Destroy this dynamic array.
		 */
		void destroy()
		{
			this->dtor();
			EGOBOO_DELETE(this);
		}

		/**
		 * @brief
		 *	Construct a dynamic array.
		 * @param initialCapacity
		 *	the initial capacity of the array
		 * @return
		 *	a pointer to this array on success, @a NULL on failure
		 * @todo
		 *	Return type should be egolib_rv.
		 * @todo
		 *	Make this a C++ constructor.
		 */
		DynamicArray<ElementType> *ctor(size_t initialCapacity)
		{
			this->top = 0;
			this->ary = EGOBOO_NEW_ARY(ElementType,initialCapacity);
			if (!this->ary)
			{
				return NULL;
			}
			this->cp = initialCapacity;
			return this;
		}

		/**
		 * @brief
		 *	Destruct this dynamic array.
		 * @todo
		 *	Make this a C++ destructor.
		 * @todo
		 *	Return type should be egolib_rv.
		 */
		void dtor()
		{
			EGOBOO_DELETE_ARY(this->ary);
			this->cp = 0;
			this->top = 0;
		}

		/// @brief Remove all elements from this array.
		void clear()
		{
			this->top = 0;
		}

		/// @brief  Get if this array is empty.
		/// @return @a true if this array is empty, @a false otherwise
		bool empty() const
		{
			return 0 == size();
		}

		/// @brief Get if this array is full.
		/// @return @a true if this array is full, @a false otherwise
		bool full() const
		{
			return size() == capacity();
		}

		/// @brief  Get the size of this array.
		/// @return the size of this array
		size_t size() const
		{
			return this->top;
		}

		/// @brief  Get the capacity of this array.
		/// @return the capacity of this array
		size_t capacity() const
		{
			return this->cp;
		}

		/// @brief  Pop an element.
		/// @return a pointer to the last element of the array if this array is not empty,
		///         @a NULL otherwise
		ElementType *pop_back()
		{
			if (empty())
			{
				return NULL;
			}
			return &(this->ary[--this->top]);
		}
		
		/// @brief  Append an element.
		/// @param  value the element
		/// @return #rv_success on success, #rv_fail on failure
		egolib_rv push_back(ElementType value)
		{
			if (this->top >= 0 && (size_t)this->top < this->cp)
			{
				this->ary[this->top++] = value;
				return rv_success;
			}
			return rv_fail;
		}

	};
};

/** @todo Remove this. */
#define DYNAMIC_ARY_INIT_VALS {0,0,NULL}
