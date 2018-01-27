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
/// @brief Canonical identifiers for vertex format descriptors.
/// @author Michael Heilmann

#pragma once

#include "egolib/integrations/idlib.hpp"

namespace Ego {

struct VertexFormatFactory {
    /// @brief Get the vertex descriptor for a vertex format.
    /// @param vertexFormat the vertex format
    /// @return the vertex descriptor for the vertex format
    static const VertexDescriptor& get(idlib::vertex_format vertexFormat);
};

template <idlib::vertex_format>
struct descriptor_factory;

template <>
struct descriptor_factory<idlib::vertex_format::P2F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P2FT2F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P3F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P3FC4F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P3FT2F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P3FC4FN3F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P3FC4FT2F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P3FC4FT2FN3F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

template <>
struct descriptor_factory<idlib::vertex_format::P3FC3FT2F>
{
    descriptor_factory() = default;
    const VertexDescriptor& operator()() const;
};

} // namespace Ego
