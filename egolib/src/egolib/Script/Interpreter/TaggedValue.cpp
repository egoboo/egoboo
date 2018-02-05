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

/// @file   egolib/Script/Interpreter/TaggdValue.cpp
/// @brief  A tagged value.
/// @author Michael Heilmann

#include "egolib/Script/Interpreter/TaggedValue.hpp"

#include "egolib/Script/Interpreter/Tag.hpp"
#include "egolib/Script/Interpreter/InvalidCastException.hpp"

namespace Ego {
namespace Script {
namespace Interpreter {

TaggedValue::TaggedValue(const TaggedValue& other) : tag(other.tag) {
    switch (tag) {
        case Tag::Boolean: booleanValue = other.booleanValue; break;
        case Tag::Integer: integerValue = other.integerValue; break;
        case Tag::Object: objectValue = other.objectValue; break;
        case Tag::Real: realValue = other.realValue; break;
        case Tag::Vector2: vector2Value = other.vector2Value; break;
        case Tag::Vector3: vector3Value = other.vector3Value; break;
        case Tag::Void: voidValue = other.voidValue; break;
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
		case Tag::EnchantProfileRef: enchantProfileRefValue = other.enchantProfileRefValue; break;
		case Tag::ObjectProfileRef: objectProfileRefValue = other.objectProfileRefValue; break;
		case Tag::ParticleProfileRef: particleProfileRefValue = other.particleProfileRefValue; break;
#endif
        default:
        {
            tag = Tag::Void;
            voidValue = VoidValue();
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
        }
    };
}

TaggedValue::TaggedValue() : tag(Tag::Void), voidValue() {}

TaggedValue::TaggedValue(BooleanValue other) : tag(Tag::Boolean), booleanValue(other) {}

TaggedValue::TaggedValue(IntegerValue other) : tag(Tag::Integer), integerValue(other) {}

TaggedValue::TaggedValue(ObjectValue other) : tag(Tag::Object), objectValue(other) {}

TaggedValue::TaggedValue(RealValue other) : tag(Tag::Real), realValue(other) {}

TaggedValue::TaggedValue(Vector2Value other) : tag(Tag::Vector2), vector2Value(other) {}

TaggedValue::TaggedValue(Vector3Value other) : tag(Tag::Vector3), vector3Value(other) {}

TaggedValue::TaggedValue(VoidValue other) : tag(Tag::Void), voidValue(other) {}

#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs

TaggedValue::TaggedValue(EnchantProfileRefValue other) : tag(Tag::EnchantProfileRef), enchantProfileRefValue(other) {}

TaggedValue::TaggedValue(ParticleProfileRefValue other) : tag(Tag::ParticleProfileRef), particleProfileRefValue(other) {}

TaggedValue::TaggedValue(ObjectProfileRefValue other) : tag(Tag::ObjectProfileRef), objectProfileRefValue(other) {}

#endif

TaggedValue::operator BooleanValue() const {
    if (Tag::Boolean != tag) {
        throw InvalidCastException(tag, Tag::Boolean);
    }
    return booleanValue;
}

TaggedValue::operator IntegerValue() const {
    if (WithImplicitConversions) {
        if (Tag::Integer == tag) return integerValue;
        else if (Tag::Real == tag) return realValue;
        else throw InvalidCastException(tag, Tag::Real);
    } else {
        if (Tag::Integer != tag) {
            throw InvalidCastException(tag, Tag::Integer);
        }
        return integerValue;
    }
}

TaggedValue::operator ObjectValue() const {
    if (Tag::Object != tag) {
        throw InvalidCastException(tag, Tag::Object);
    }
    return objectValue;
}

TaggedValue::operator RealValue() const {
    if (WithImplicitConversions) {
        if (Tag::Integer == tag) return integerValue;
        else if (Tag::Real == tag) return realValue;
        else throw InvalidCastException(tag, Tag::Real);
    } else {
        if (Tag::Real != tag) {
            throw InvalidCastException(tag, Tag::Real);
        }
        return realValue;
    }
}

TaggedValue::operator Vector2Value() const {
    if (Tag::Vector2 != tag) {
        throw InvalidCastException(tag, Tag::Vector2);
    }
    return vector2Value;
}

TaggedValue::operator Vector3Value() const {
    if (Tag::Vector3 != tag) {
        throw InvalidCastException(tag, Tag::Vector3);
    }
    return vector3Value;
}

TaggedValue::operator VoidValue() const {
    if (Tag::Void != tag) {
        throw InvalidCastException(tag, Tag::Void);
    }
    return voidValue;
}

#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs

TaggedValue::operator EnchantProfileRefValue() const {
    if (Tag::EnchantProfileRef != tag) {
        throw InvalidCastException(tag, Tag::EnchantProfileRef);
    }
    return enchantProfileRefValue;
}

TaggedValue::operator ParticleProfileRefValue() const {
    if (Tag::ParticleProfileRef != tag) {
        throw InvalidCastException(tag, Tag::ParticleProfileRef);
    }
    return particleProfileRefValue;
}

TaggedValue::operator ObjectProfileRefValue() const {
    if (Tag::ObjectProfileRef != tag) {
        throw InvalidCastException(tag, Tag::ObjectProfileRef);
    }
    return objectProfileRefValue;
}

#endif

void TaggedValue::destructValue() {
    switch (tag) {
        case Tag::Boolean:
        case Tag::Integer:
        case Tag::Real:
            break;
        case Tag::Object:
            objectValue.~ObjectValue();
            break;
        case Tag::Vector2:
            vector2Value.~Vector2Value();
            break;
        case Tag::Vector3:
            vector3Value.~Vector3Value();
            break;
        case Tag::Void:
            break;
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
		case Tag::EnchantProfileRef:
			enchantProfileRefValue.~EnchantProfileRefValue();
			break;
		case Tag::ParticleProfileRef:
			particleProfileRefValue.~ParticleProfileRefValue();
			break;
		case Tag::ObjectProfileRef:
			objectProfileRefValue.~ObjectProfileRefValue();
			break;
#endif
    };
}

TaggedValue& TaggedValue::operator=(const TaggedValue& other) {
    switch (tag) {
        case Tag::Boolean: booleanValue = other.booleanValue; break;
        case Tag::Integer: integerValue = other.integerValue; break;
        case Tag::Object: objectValue = other.objectValue; break;
        case Tag::Real: realValue = other.realValue; break;
        case Tag::Vector2: vector2Value = other.vector2Value; break;
        case Tag::Vector3: vector3Value = other.vector3Value; break;
        case Tag::Void: voidValue = other.voidValue; break;
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
        case Tag::EnchantProfileRef: enchantProfileRefValue = other.enchantProfileRefValue; break;
        case Tag::ParticleProfileRef: particleProfileRefValue = other.particleProfileRefValue; break;
        case Tag::ObjectProfileRef: objectProfileRefValue = other.objectProfileRefValue; break;
#endif
        default:
        {
            tag = Tag::Void;
            voidValue = VoidValue();
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
        }
    };
    tag = other.tag;
    return *this;
}

TaggedValue& TaggedValue::operator=(BooleanValue other) {
    destructValue();
    tag = Tag::Boolean;
    booleanValue = other;
    return *this;
}

TaggedValue& TaggedValue::operator=(IntegerValue other) {
    destructValue();
    tag = Tag::Integer;
    integerValue = other;
    return *this;
}

TaggedValue& TaggedValue::operator=(RealValue other) {
    destructValue();
    tag = Tag::Real;
    realValue = other;
    return *this;
}

TaggedValue& TaggedValue::operator=(Vector2Value other) {
    destructValue();
    tag = Tag::Vector2;
    vector2Value = other;
    return *this;
}

TaggedValue& TaggedValue::operator=(Vector3Value other) {
    destructValue();
    tag = Tag::Vector3;
    vector3Value = other;
    return *this;
}

TaggedValue& TaggedValue::operator=(VoidValue other) {
    destructValue();
    tag = Tag::Void;
    voidValue = other;
    return *this;
}

#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs

TaggedValue& TaggedValue::operator=(EnchantProfileRefValue other) {
    destructValue();
    tag = Tag::EnchantProfileRef;
    enchantProfileRefValue = other;
    return *this;
}

TaggedValue& TaggedValue::operator=(ParticleProfileRefValue other) {
    destructValue();
    tag = Tag::ParticleProfileRef;
    particleProfileRefValue = other;
    return *this;
}

TaggedValue& TaggedValue::operator=(ObjectProfileRefValue other) {
    destructValue();
    tag = Tag::ObjectProfileRef;
    objectProfileRefValue = other;
    return *this;
}

#endif

TaggedValue::~TaggedValue() {
    destructValue();
}

Tag TaggedValue::getTag() const {
    return tag;
}

} // namespace Interpreter
} // namespace Script
} // namespace Ego

std::ostream& operator<<(std::ostream& os, const Ego::Script::Interpreter::TaggedValue& taggedValue) {
    os << toString(taggedValue.getTag());
    switch (taggedValue.getTag()) {
        case Ego::Script::Interpreter::Tag::Boolean: os << taggedValue.booleanValue; break;
        case Ego::Script::Interpreter::Tag::Integer: os << taggedValue.integerValue; break;
        case Ego::Script::Interpreter::Tag::Object: os << taggedValue.objectValue; break;
        case Ego::Script::Interpreter::Tag::Real: os << taggedValue.realValue; break;
        case Ego::Script::Interpreter::Tag::Vector2: os << taggedValue.vector2Value; break;
        case Ego::Script::Interpreter::Tag::Vector3: os << taggedValue.vector3Value; break;
        case Ego::Script::Interpreter::Tag::Void: os << taggedValue.voidValue; break;
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
        case Ego::Script::Interpreter::Tag::EnchantProfileRef: os << taggedValue.enchantProfileRefValue; break;
        case Ego::Script::Interpreter::Tag::ParticleProfileRef: os << taggedValue.particleProfileRefValue; break;
        case Ego::Script::Interpreter::Tag::ObjectProfileRef: os << taggedValue.objectProfileRefValue; break;
#endif
        default: {
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
        }
    };
    return os;
}
