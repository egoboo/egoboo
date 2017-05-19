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

namespace Ego {

template <typename EnumType, EnumType EnumElementType>
struct Generator;

template <>
struct Generator<IndexFormat, IndexFormat::IU8> {
    Generator() {}
    const IndexDescriptor& operator()() const {
        static const IndexDescriptor descriptor(IndexDescriptor::Syntax::U8);
        return descriptor;
    }
};

template <>
struct Generator<IndexFormat, IndexFormat::IU16> {
    Generator() {}
    const IndexDescriptor& operator()() const {
        static const IndexDescriptor descriptor(IndexDescriptor::Syntax::U16);
        return descriptor;
    }
};

template <>
struct Generator<IndexFormat, IndexFormat::IU32> {
    Generator() {}
    const IndexDescriptor& operator()() const {
        static const IndexDescriptor descriptor(IndexDescriptor::Syntax::U32);
        return descriptor;
    }
};

template <>
struct Generator<VertexFormat, VertexFormat::P2F> {
    Generator() {}
	const VertexDescriptor& operator()() const {
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Position);
		static const VertexDescriptor descriptor({position});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P2FT2F> {
    Generator() {}
	const VertexDescriptor& operator()() const {
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Position);
		static const VertexElementDescriptor texture(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
		static const VertexDescriptor descriptor({position, texture});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P3F> {
    Generator() {}
	const VertexDescriptor& operator()() const {
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
		static const VertexDescriptor descriptor({position});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P3FC4F> {
    Generator() {}
	const VertexDescriptor& operator()() const {	
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
		static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
		static const VertexDescriptor descriptor({position, colour});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P3FT2F> {
    Generator() {}
	const VertexDescriptor& operator()() const {
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
		static const VertexElementDescriptor texture(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
		static const VertexDescriptor descriptor({position, texture});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P3FC4FN3F> {
    Generator() {}
	const VertexDescriptor& operator()() const {
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
		static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
		static const VertexElementDescriptor normal(colour.getOffset() + colour.getSize(), VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Normal);
		static const VertexDescriptor descriptor({position, colour, normal});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P3FC4FT2F> {
    Generator() {}
	const VertexDescriptor& operator()() const {
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
		static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
		static const VertexElementDescriptor texture(colour.getOffset() + colour.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
		static const VertexDescriptor descriptor({position, colour, texture});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P3FC4FT2FN3F> {
    Generator() {}
	const VertexDescriptor& operator()() const {
		static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
		static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
		static const VertexElementDescriptor texture(colour.getOffset() + colour.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
		static const VertexElementDescriptor normal(texture.getOffset() + texture.getSize(), VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Normal);
		static const VertexDescriptor descriptor({position, colour, texture, normal});
		return descriptor;
	}
};

template <>
struct Generator<VertexFormat, VertexFormat::P3FC3FT2F> {
    Generator() {}
    const VertexDescriptor& operator()() const {
        static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
        static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Colour);
        static const VertexElementDescriptor texture(colour.getOffset() + colour.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
        static const VertexDescriptor descriptor({position, colour, texture});
        return descriptor;
    }
};

const IndexDescriptor& IndexFormatFactory::get(IndexFormat indexFormat) {
    switch (indexFormat) {
        case IndexFormat::IU32:
        { static const Generator<IndexFormat, IndexFormat::IU32> g; return g(); }
        case IndexFormat::IU16:
        { static const Generator<IndexFormat, IndexFormat::IU16> g; return g(); }
        case IndexFormat::IU8:
        { static const Generator<IndexFormat, IndexFormat::IU8> g; return g(); }
        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

template <>
const IndexDescriptor& IndexFormatFactory::get<IndexFormat::IU8>() {
    static const Generator<IndexFormat, IndexFormat::IU8> g;
    return g();
}

template <>
const IndexDescriptor& IndexFormatFactory::get<IndexFormat::IU16>() {
    static const Generator<IndexFormat, IndexFormat::IU16> g;
    return g();
}

template <>
const IndexDescriptor& IndexFormatFactory::get<IndexFormat::IU32>() {
    static const Generator<IndexFormat, IndexFormat::IU32> g;
    return g();
}

const VertexDescriptor& VertexFormatFactory::get(VertexFormat vertexFormat) {
    switch (vertexFormat) {
        case VertexFormat::P2F:
        { static const Generator<VertexFormat, VertexFormat::P2F> g{}; return g(); }
		case VertexFormat::P2FT2F:
        { static const Generator<VertexFormat, VertexFormat::P2FT2F> g{}; return g(); }
        case VertexFormat::P3F:
        { static const Generator<VertexFormat, VertexFormat::P3F> g{}; return g(); }
        case VertexFormat::P3FT2F:
        { static const Generator<VertexFormat, VertexFormat::P3FT2F> g{}; return g(); }
        case VertexFormat::P3FC4F:
        { static const Generator<VertexFormat, VertexFormat::P3FC4F> g{}; return g(); }
        case VertexFormat::P3FC4FN3F:
        { static const Generator<VertexFormat, VertexFormat::P3FC4FN3F> g{}; return g(); }
        case VertexFormat::P3FC4FT2F:
        { static const Generator<VertexFormat, VertexFormat::P3FC4FT2F> g{}; return g(); }
        case VertexFormat::P3FC4FT2FN3F:
        { static const Generator<VertexFormat, VertexFormat::P3FC4FT2FN3F> g{}; return g(); }
        case VertexFormat::P3FC3FT2F:
        { static const Generator<VertexFormat, VertexFormat::P3FC3FT2F> g{}; return g(); }
        default:
			throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P2F>() {
    static const Generator<VertexFormat, VertexFormat::P2F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P2FT2F>() {
    static const Generator<VertexFormat, VertexFormat::P2FT2F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3F>() {
    static const Generator<VertexFormat, VertexFormat::P3F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4F>() {
    static const Generator<VertexFormat, VertexFormat::P3FC4F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FT2F>() {
    static const Generator<VertexFormat, VertexFormat::P3FT2F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4FN3F>() {
    static const Generator<VertexFormat, VertexFormat::P3FC4FN3F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4FT2F>() {
    static const Generator<VertexFormat, VertexFormat::P3FC4FT2F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC4FT2FN3F>() {
    static const Generator<VertexFormat, VertexFormat::P3FC4FT2FN3F> g;
	return g();
}

template <>
const VertexDescriptor& VertexFormatFactory::get<VertexFormat::P3FC3FT2F>() {
    static const Generator<VertexFormat, VertexFormat::P3FC3FT2F> g;
    return g();
}

} // namespace Ego
