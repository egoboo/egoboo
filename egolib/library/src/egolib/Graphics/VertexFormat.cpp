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

/// @file egolib/Graphics/VertexFormat.cpp
/// @brief Canonical identifiers for vertex format descriptors.
/// @author Michael Heilmann

#include "egolib/Graphics/VertexFormat.hpp"
#include "egolib/Graphics/descriptor_factory.hpp"

namespace Ego {

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P2F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_descriptor descriptor({ position });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P2FT2F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_component_descriptor texture(position.get_offset() + position.get_size(), idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::TEXTURE);
    static const idlib::vertex_descriptor descriptor({ position, texture });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P3F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_descriptor descriptor({ position });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P3FC4F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_component_descriptor colour(position.get_offset() + position.get_size(), idlib::vertex_component_syntactics::SINGLE_4, idlib::vertex_component_semantics::COLOR);
    static const idlib::vertex_descriptor descriptor({ position, colour });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P3FT2F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_component_descriptor texture(position.get_offset() + position.get_size(), idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::TEXTURE);
    static const idlib::vertex_descriptor descriptor({ position, texture });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P3FC4FN3F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_component_descriptor colour(position.get_offset() + position.get_size(), idlib::vertex_component_syntactics::SINGLE_4, idlib::vertex_component_semantics::COLOR);
    static const idlib::vertex_component_descriptor normal(colour.get_offset() + colour.get_size(), idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::NORMAL);
    static const idlib::vertex_descriptor descriptor({ position, colour, normal });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P3FC4FT2F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_component_descriptor colour(position.get_offset() + position.get_size(), idlib::vertex_component_syntactics::SINGLE_4, idlib::vertex_component_semantics::COLOR);
    static const idlib::vertex_component_descriptor texture(colour.get_offset() + colour.get_size(), idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::TEXTURE);
    static const idlib::vertex_descriptor descriptor({ position, colour, texture });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P3FC4FT2FN3F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_component_descriptor  colour(position.get_offset() + position.get_size(), idlib::vertex_component_syntactics::SINGLE_4, idlib::vertex_component_semantics::COLOR);
    static const idlib::vertex_component_descriptor  texture(colour.get_offset() + colour.get_size(), idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::TEXTURE);
    static const idlib::vertex_component_descriptor  normal(texture.get_offset() + texture.get_size(), idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::NORMAL);
    static const idlib::vertex_descriptor descriptor({ position, colour, texture, normal });
    return descriptor;
}

const idlib::vertex_descriptor& descriptor_factory<idlib::vertex_format::P3FC3FT2F>::operator()() const
{
    static const idlib::vertex_component_descriptor position(0, idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::POSITION);
    static const idlib::vertex_component_descriptor colour(position.get_offset() + position.get_size(), idlib::vertex_component_syntactics::SINGLE_3, idlib::vertex_component_semantics::COLOR);
    static const idlib::vertex_component_descriptor texture(colour.get_offset() + colour.get_size(), idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::TEXTURE);
    static const idlib::vertex_descriptor descriptor({ position, colour, texture });
    return descriptor;
}

const idlib::vertex_descriptor& VertexFormatFactory::get(idlib::vertex_format vertexFormat)
{
    switch (vertexFormat)
    {
    case idlib::vertex_format::P2F:
    { return descriptor_factory<idlib::vertex_format::P2F>()(); }
    case idlib::vertex_format::P2FT2F:
    { return descriptor_factory<idlib::vertex_format::P2FT2F>()(); }
    case idlib::vertex_format::P3F:
    { return descriptor_factory<idlib::vertex_format::P3F>()(); }
    case idlib::vertex_format::P3FT2F:
    { return descriptor_factory<idlib::vertex_format::P3FT2F>()(); }
    case idlib::vertex_format::P3FC4F:
    { return descriptor_factory<idlib::vertex_format::P3FC4F>()(); }
    case idlib::vertex_format::P3FC4FN3F:
    { return descriptor_factory<idlib::vertex_format::P3FC4FN3F>()(); }
    case idlib::vertex_format::P3FC4FT2F:
    { return descriptor_factory<idlib::vertex_format::P3FC4FT2F>()(); }
    case idlib::vertex_format::P3FC4FT2FN3F:
    { return descriptor_factory<idlib::vertex_format::P3FC4FT2FN3F>()(); }
    case idlib::vertex_format::P3FC3FT2F:
    { return descriptor_factory<idlib::vertex_format::P3FC3FT2F>()(); }
    default:
        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

} // namespace Ego
