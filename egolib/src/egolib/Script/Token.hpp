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

/// @file   egolib/Script/Token.cpp
/// @brief  A basic token class
/// @author Michael Heilmann

#pragma once

#include "egolib/typedef.h"
#include "egolib/Script/Conversion.hpp"

namespace Ego {
namespace Script {

/**
 * @todo
 *  Rename to Ego::Script::Token.
 *  However, this is not possible at as EgoScript defines its *own* token type.
 */
struct TextToken {
	enum Type {
		/**
		 * @brief
		 *  An integer number literal.
		 */
		Integer,
		/**
		 * @brief
		 *	A real number literal.
		 */
		Real,
		/**
		 * @brief
		 *  A character literal.
		 */
		Character,
		/**
		 * @brief
		 *  A string literal.
		 */
		String,
		/**
		 * @brief
		 *  A boolean literal.
		 */
		Boolean,
		/**
		 * @brief
		 *  A name (aka identifier).
		 */
		Name,
		/**
		 * @brief
		 *  A comment.
		 */
		Comment,
	};
private:
	/**
	 * @brief
	 *  The type of this token.
	 */
	Type _type;
	/**
	 * @brief
	 *  The location of this token.
	 */
	Id::Location _location;
	/**
	 * @brief
	 *  The lexeme of this token.
	 */
	std::string _lexeme;
public:
	/**
	 * @brief
	 *  Construct this token.
	 * @param type
	 *  the type of this token
	 * @param location
	 *  the location of this token
	 * @param lexeme
	 *  the lexeme of this token. Default is the empty string.
	 */
	TextToken(Type type, const Id::Location& location, const std::string& lexeme = std::string());
	/**
	 * @brief
	 *  Copy-construct this token from another token.
	 * @param other
	 *  the construction source
	 */
	TextToken(const TextToken& other);
	/**
	 * @brief
	 *  Get the type of this token.
	 * @return
	 *  the type of this token
	 */
	Type getType() const;
	/**
	 * @brief
	 *  Get the location of this token.
	 * @return
	 *  the location of this token
	 */
	const Id::Location& getLocation() const;
	/**
	 * @brief
	 *  Assign this token from another token.
	 * @param other
	 *  the construction source
	 */
	TextToken& operator=(const TextToken& other);
	/**
	 * @brief
	 *  Get the lexeme of this token.
	 * @return
	 *  the lexeme of this token
	 */
	const std::string& getLexeme() const;

};

/**
 * @brief
 *  Convert this token into a value of type @a type.
 * @tparam Type
 *  the type of the value to convert the value of this token into
 * @return
 *  the converted value
 * @throw Id::LexicalErrorException
 *  if conversion fails
 */
template <typename TargetType, typename EnabledType = void>
struct TextTokenDecoder {
	TargetType operator()(const TextToken& token) const;
};

/**
 * @brief Specialization for EgoScript boolean types.
 */
template <typename TargetType>
struct TextTokenDecoder<TargetType, std::enable_if_t<IsBoolean<TargetType>::value>> {
    TargetType operator()(const TextToken& token) const {
        TargetType temporary;
        if (!Decoder<TargetType>()(token.getLexeme(), temporary)) {
            throw Id::LexicalErrorException(__FILE__, __LINE__, token.getLocation(),
                                            "unable to convert literal `" + token.getLexeme() +
                                            "` into a value of EgoScript boolean type `" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/**
 * @brief Specialization for EgoScript character types.
 */
template <typename TargetType>
struct TextTokenDecoder<TargetType, std::enable_if_t<IsCharacter<TargetType>::value>> {
    TargetType operator()(const TextToken& token) const {
        TargetType temporary;
		if (!Decoder<TargetType>()(token.getLexeme(), temporary)) {
			throw Id::LexicalErrorException(__FILE__, __LINE__, token.getLocation(),
				                            "unable to convert literal `" + token.getLexeme() +
				                            "` into a value of EgoScript character type `" + typeid(TargetType).name() + "`");
		}
		return temporary;
	}
};

/**
 * @brief Specialization for EgoScript real types.
 */
template <typename TargetType>
struct TextTokenDecoder<TargetType, std::enable_if_t<IsReal<TargetType>::value>> {
    TargetType operator()(const TextToken& token) const {
		TargetType temporary;
		if (!Decoder<TargetType>()(token.getLexeme(), temporary)) {
			throw Id::LexicalErrorException(__FILE__, __LINE__, token.getLocation(),
				                            "unable to convert literal `" + token.getLexeme() +
				                            "` into a value of EgoScript real type " + "`" + typeid(TargetType).name() + "`");
		}
		return temporary;
	}
};

/**
 * @brief Specialization for EgoScript integer types.
 */
template <typename TargetType>
struct TextTokenDecoder<TargetType, std::enable_if_t<IsInteger<TargetType>::value>> {
    TargetType operator()(const TextToken& token) const {
        TargetType temporary;
		if (!Decoder<TargetType>()(token.getLexeme(), temporary)) {
			throw Id::LexicalErrorException(__FILE__, __LINE__, token.getLocation(),
				                            "unable to convert literal `" + token.getLexeme() +
				                            "` into a value of EgoScript integer type " + "`" + typeid(TargetType).name() + "`");
		}
		return temporary;
	}
};

/**
 * @brief Specialization for EgoScript natural types.
 */
template<typename TargetType>
struct TextTokenDecoder<TargetType, std::enable_if_t<IsNatural<TargetType>::value>> {
    TargetType operator()(const TextToken& token) const {
		TargetType temporary;
		if (!Decoder<TargetType>()(token.getLexeme(), temporary)) {
			throw Id::LexicalErrorException(__FILE__, __LINE__, token.getLocation(),
				                            "unable to convert literal `" + token.getLexeme() +
				                            "` into a value of EgoScript natural type " + "`" + typeid(int).name() + "`");
		}
		return temporary;
	}
};

/** 
 * @brief Specialization for EgoScript string types.
 */
template <typename TargetType>
struct TextTokenDecoder<TargetType, std::enable_if_t<IsString<TargetType>::value>> {
    TargetType operator()(const TextToken& token) const {
        TargetType temporary;
		if (!Decoder<TargetType>()(token.getLexeme(), temporary)) {
			throw Id::LexicalErrorException(__FILE__, __LINE__, token.getLocation(),
				                            "unable to convert literal `" + token.getLexeme() +
				                            "` into a value of EgoScript string type " + "`" + typeid(TargetType).name() + "`");
		}
		return temporary;
	}
};

} // namespace Script
} // namespace Ego
