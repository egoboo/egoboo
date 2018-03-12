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

/// @file egolib/Script/Buffer.cpp
/// @brief Dynamically resizing buffer for bytes.
/// @author Michael Heilmann

#include "egolib/Script/Buffer.hpp"

namespace Ego::Script {

Buffer::Buffer() :
    elements()
{}


Buffer::~Buffer()
{}

size_t Buffer::getSize() const
{
    return elements.size();
}

char Buffer::get(size_t index) const
{
    return elements[index];
}

void Buffer::clear()
{
    elements.clear();
}

std::string Buffer::toString() const
{
    return std::string(elements.cbegin(), elements.cend());
}

void Buffer::prepend(char byte)
{
    elements.push_front(byte);
}

void Buffer::prepend(const char *bytes, size_t numberOfBytes)
{
    if (0 == numberOfBytes) return;
    elements.insert(elements.begin(), bytes, bytes + numberOfBytes);
}

void Buffer::append(char byte)
{
    elements.push_back(byte);
}

void Buffer::append(const char *bytes, size_t numberOfBytes)
{
    if (0 == numberOfBytes) return;
    elements.insert(elements.end(), bytes, bytes + numberOfBytes);
}

void Buffer::insert(char byte, size_t index)
{
    elements.insert(elements.begin() + index, byte);
}

bool Buffer::isEmpty() const
{
    return 0 == getSize();
}

void Buffer::swap(Buffer& other) noexcept
{
    std::swap(elements, other.elements);
}

} // namespace Ego::Script
