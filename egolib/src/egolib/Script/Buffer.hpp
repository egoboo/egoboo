#pragma once

#include "egolib/platform.h"

namespace Ego
{
    namespace Script
    {

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
            Buffer(size_t initialCapacity);

            /**
             * @brief
             *	Destruct this buffer.
             */
            ~Buffer();

            /**
             * @brief
             *  Get the size of this buffer.
             * @return
             *  the size
             */
            size_t getSize() const;

            /**
             * @brief
             *  Get the capacity of this buffer.
             * @return
             *  the capacity
             */
            size_t getCapacity() const;

            /**
             * @brief
             *  Get the maximum capacity of this buffer.
             * @return
             *  the maximum capacity
             */
            size_t getMaxCapacity() const;

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
            void increaseCapacity(size_t req);

            /**
             * @brief
             *  Clear this buffer.
             */
            void clear();

            /**
             * @brief
             *	Append a byte to the buffer growing the buffer if necessary.
             * @param byte
             *	the byte
             * @throw std::bad_alloc
             *	if not enough memory is available
             */
            void append(char byte);

        };

    }
}