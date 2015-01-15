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

		/// @todo   Make this a C+++ constructor.
		/// @param  initialCapacity the initial capacity of the array
		/// @return a pointer to this array on success, @a NULL on failure
		/// @todo   Return type should be egolib_rv.
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

		/// @todo Make this a C++ destructor.
		/// @todo Return type should be egolib_rv.
		DynamicArray<ElementType> *dtor()
		{
			EGOBOO_DELETE_ARY(this->ary);
			this->cp = 0;
			this->top = 0;
			return this;
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


/// A template-like declaration of a dynamically allocated array.
#define DECLARE_DYNAMIC_ARY(ARY_T, ELEM_T)                   \
	typedef Ego::DynamicArray<ELEM_T> ARY_T##_t;             \
	                                                         \
    ARY_T##_t *ARY_T##_ctor(ARY_T##_t *pary, size_t cp);     \
    ARY_T##_t *ARY_T##_dtor(ARY_T##_t *pary);                \
    void ARY_T##_clear(ARY_T##_t *pary);                     \
    size_t ARY_T##_get_top(const ARY_T##_t *pary);           \
    size_t ARY_T##_get_cp(const ARY_T##_t *pary);            \
    ELEM_T *ARY_T##_pop_back(ARY_T##_t *pary);               \
    egolib_rv ARY_T##_push_back(ARY_T##_t *pary , ELEM_T val);

#define DYNAMIC_ARY_INIT_VALS {0,0,NULL}

#if 0
#define INSTANTIATE_DYNAMIC_ARY(ARY_T, NAME) ARY_T##_t NAME = DYNAMIC_ARY_INIT_VALS;
#endif

#define IMPLEMENT_DYNAMIC_ARY(ARY_T, ELEM_T) \
    ARY_T##_t *ARY_T##_ctor(ARY_T##_t *self, size_t initialCapacity) \
	{ \
		if (NULL == self) \
		{ \
			return NULL; \
		} \
		return self->ctor(initialCapacity); \
	} \
    ARY_T##_t *ARY_T##_dtor(ARY_T##_t *self) \
	{ \
		if (NULL == self) \
		{ \
			return NULL; \
		} \
		return self->dtor(); \
	} \
    void ARY_T##_clear(ARY_T##_t *self) \
	{ \
		if (NULL != self) \
		{ \
			self->clear(); \
		} \
	} \
    size_t ARY_T##_get_top(const ARY_T##_t *self) \
	{ \
		if (NULL == self) \
		{ \
			return 0; \
		} \
		return (NULL == self->ary) ? 0 : self->size(); \
	} \
    size_t ARY_T##_get_cp(const ARY_T##_t *pary)          { return (NULL == pary->ary) ? 0 : pary->capacity(); }     \
    \
    ELEM_T *ARY_T##_pop_back(ARY_T##_t *pary)             { if (NULL == pary) return NULL; return pary->pop_back(); } \
    egolib_rv ARY_T##_push_back(ARY_T##_t *pary, ELEM_T value) { if (NULL == pary) return rv_fail; return pary->push_back(value); }

#define DYNAMIC_ARY_INVALID_RAW(PARY) ( (0 == (PARY)->cp) || ((PARY)->top < 0) || ((size_t)(PARY)->top >= (PARY)->cp) )
#define DYNAMIC_ARY_INVALID(PARY) ( (NULL == (PARY)) || DYNAMIC_ARY_INVALID_RAW(PARY) )

#define DYNAMIC_ARY_VALID_RAW(PARY) ( ((PARY)->cp > 0) && ((PARY)->top >= 0) && ((size_t)(PARY)->top < (PARY)->cp) )
#define DYNAMIC_ARY_VALID(PARY) ( (NULL != (PARY)) && DYNAMIC_ARY_VALID_RAW(PARY) )

// a NULL, invalid, or empty list are all "empty"
#define DYNAMIC_ARY_HAS_ELEMENTS_RAW(PARY) ( ((PARY)->cp > 0) && ((PARY)->top > 0) && (((size_t)(PARY)->top) < (PARY)->cp) )
#define DYNAMIC_ARY_HAS_ELEMENTS(PARY) ( (NULL != (PARY)) && DYNAMIC_ARY_HAS_ELEMENTS_RAW(PARY) )

// only valid lists can be full
// avoid subtraction from unsigned values
#define DYNAMIC_ARY_CAN_ADD_ELEMENTS_RAW(PARY) ( ((PARY)->cp > 0) && ((PARY)->top >= 0) && (((size_t)(PARY)->top) + 1 < (PARY)->cp) )
#define DYNAMIC_ARY_CAN_ADD_ELEMENTS(PARY) ( (NULL != (PARY)) && DYNAMIC_ARY_CAN_ADD_ELEMENTS_RAW(PARY) )
