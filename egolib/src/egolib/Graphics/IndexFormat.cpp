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

/// @file egolib/Graphics/IndexFormat.cpp
/// @brief Canonical identifiers for index format descriptors.
/// @author Michael Heilmann

#include "egolib/Graphics/IndexFormat.hpp"
#include "egolib/Graphics/descriptor_factory.hpp"

namespace Ego {

const IndexDescriptor& descriptor_factory<idlib::index_format::IU8>::operator()() const
{
    static const IndexDescriptor descriptor(idlib::index_syntactics::NATURAL_8);
    return descriptor;
}

const IndexDescriptor& descriptor_factory<idlib::index_format::IU16>::operator()() const
{
    static const IndexDescriptor descriptor(idlib::index_syntactics::NATURAL_16);
    return descriptor;
}

const IndexDescriptor& descriptor_factory<idlib::index_format::IU32>::operator()() const
{
    static const IndexDescriptor descriptor(idlib::index_syntactics::NATURAL_32);
    return descriptor;
}

const IndexDescriptor& IndexFormatFactory::get(idlib::index_format indexFormat)
{
    switch (indexFormat)
    {
    case idlib::index_format::IU32:
    { return descriptor_factory<idlib::index_format::IU32>()(); }
    case idlib::index_format::IU16:
    { return descriptor_factory<idlib::index_format::IU16>()(); }
    case idlib::index_format::IU8:
    { return descriptor_factory<idlib::index_format::IU8>()(); }
    default:
        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

} // namespace Ego
