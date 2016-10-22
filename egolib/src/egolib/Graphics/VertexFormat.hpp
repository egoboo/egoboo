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

#include "egolib/Graphics/IndexDescriptor.hpp"
#include "egolib/Graphics/VertexDescriptor.hpp"

namespace Ego {

/// @brief Enumeration of canonical identifiers for index formats.
enum class IndexFormat {
    /// @brief One unsigned 32 Bit value for the index component.
    IU32,

    /// @brief One unsigned 16 Bit value for the index component.
    IU16,

    /// @brief One unsigned 8 Bit value for the index component.
    IU8,
};

/// @brief Enumeration of canonical identifiers for vertex formats.
enum class VertexFormat {
    /// @brief
    /// Two floats for the position component.
    P2F,

	/// @brief
	/// Two floats for the position component,
	/// two floats for the texture component.
	P2FT2F,

    /// @brief
    /// Three floats for the position component.
    P3F,

    /// @brief
    /// Three floats for the position component and
    /// four floats for colour component.
    P3FC4F,

    /// @brief
    /// Three floats for the position component and
    /// two floats for the texture component.
    P3FT2F,

    /// @brief
    /// Three floats for the position component,
    /// four floats for the colour component, and
    /// three floats for the normal component.
    P3FC4FN3F,

    /// @brief
    /// Three floats for the position component,
    /// four floats for the colour component, and
    /// two floats for the texture component.
    P3FC4FT2F,

    /// @brief
    /// Three floats for the position component,
    /// four floats for the colour component,
    /// two floats for the texture component, and
    /// three floats for the normal component.
    P3FC4FT2FN3F,

    /// @brief
    /// Three floats for the position component,
    /// three floats for the colour component, and
    /// two floats for the texture component.
    P3FC3FT2F,

}; // enum class VertexFormat

struct IndexFormatFactory {
    /// @brief Get the index descriptor for an index format.
    /// @param indexFormat the index format
    /// @return the index descriptor for the index format
    static const IndexDescriptor& get(IndexFormat indexFormat);

    template <IndexFormat IndexFormat>
    static const IndexDescriptor& get();
};

template <>
const IndexDescriptor& IndexFormatFactory::get<IndexFormat::IU8>();

template <>
const IndexDescriptor& IndexFormatFactory::get<IndexFormat::IU16>();

template <>
const IndexDescriptor& IndexFormatFactory::get<IndexFormat::IU32>();

struct VertexFormatFactory {
    /// @brief Get the vertex descriptor for a vertex format.
    /// @param vertexFormat the vertex format
    /// @return the vertex descriptor for the vertex format
    static const VertexDescriptor& get(VertexFormat vertexFormat);

    template <VertexFormat VertexFormat>
    static const VertexDescriptor& get();
};

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P2F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P2FT2F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FT2F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4FN3F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4FT2F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4FT2FN3F>();

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC3FT2F>();

} // namespace Ego
