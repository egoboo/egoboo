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

/// @file egolib/Graphics/VertexFormat.hpp
/// @brief Canonical identifiers for index format descriptors.
/// @author Michael Heilmann

#pragma once

#include "egolib/integrations/video.hpp"

namespace Ego {

struct IndexFormatFactory {
    /// @brief Get the index descriptor for an index format.
    /// @param indexFormat the index format
    /// @return the index descriptor for the index format
    static const idlib::index_descriptor& get(idlib::index_format indexFormat);
};

template <idlib::index_format F>
struct descriptor_factory;

template <>
struct descriptor_factory<idlib::index_format::IU8>
{
    descriptor_factory() = default;
    const idlib::index_descriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::index_format::IU16>
{
    descriptor_factory() = default;
    const idlib::index_descriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::index_format::IU32>
{
    descriptor_factory() = default;
    const idlib::index_descriptor& operator()() const;
};

} // namespace Ego
