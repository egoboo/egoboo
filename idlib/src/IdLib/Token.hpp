#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Location.hpp"

#define Id_Token_WithEndLocation (0)

namespace Id
{

/// @brief Generic token.
template <typename KindType>
struct Token	
{
private:
	/// @brief The kind of this token.
	KindType m_kind;

	/// @brief The start location of this token.
	Location m_startLocation;

#if defined(Id_Token_WithEndLocation) && 1 == Id_Token_WithEndLocation
    /// @brief The end location of this token.
    Location m_endLocation;
#endif

	/// @brief The lexeme of this token.
	std::string m_lexeme;

public:
	/// @brief Construct this token with the specified values.
	/// @param kind the kind of this token
	/// @param startLocation the start location of this token
    /// @param endLocation the end location of this token
	/// @param lexeme the lexeme of this token. Default is the empty string.
	Token(KindType kind, const Location& startLocation,
      #if defined(Id_Token_WithEndLocation) && 1 == Id_Token_WithEndLocation  
          const Location& endLocation,
      #endif
          const std::string& lexeme = std::string())
		: m_kind(kind),
          m_startLocation(startLocation),
    #if defined(Id_Token_WithEndLocation) && 1 == Id_Token_WithEndLocation  
          m_endLocation(endLocation),
    #endif
          m_lexeme(lexeme)
	{
		/* Intentionally empty. */
	}
	
	/// @brief Copy-Construct this token from another token.
	/// @param other the other token
	Token(const Token& other)
		: m_kind(other.getKind()),
          m_startLocation(other.getStartLocation()),
    #if defined(Id_Token_WithEndLocation) && 1 == Id_Token_WithEndLocation
          m_endLocation(other.getEndLocation()),
    #endif
          m_lexeme(other.getLexeme())
	{
		/* Intentionally empty. */
	}

    /// @brief Move-construct this token from another token.
    /// @param other the other token
    Token(Token&& other)
        : m_kind(std::move(other.getKind())),
          m_startLocation(std::move(other.m_startLocation)),
    #if defined(Id_Token_WithEndLocation) && 1 == Id_Token_WithEndLocation
          m_endLocation(std::move(other.m_endLocation)),
    #endif
          m_lexeme(std::move(other.m_lexeme))
    {}

    /// @brief Assign this token from another token.
    /// @param other the other token
    /// @return this token
    Token& operator=(Token other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(Token& x, Token& y)
    {
        std::swap(x.m_kind, y.m_kind);
        std::swap(x.m_startLocation, y.m_startLocation);
    #if defined(Id_Token_WithEndLocation) && 1 == Id_Token_WithEndLocation
        std::swap(x.m_endLocation, y.m_endLocatation);
    #endif
        std::swap(x.m_lexeme, y.m_lexeme);
    }
	
public:
	/// @brief Get the kind of this token.
	/// @return the kind of this token
	const KindType& getKind() const
	{
		return m_kind;
	}
	
	/// @brief Set the kind of this token.
	/// @param kind the kind of this token
	void setKind(const KindType& kind)
	{
		m_kind = kind;
	}

#if defined(Id_Token_WithEndLocation) && 1 == Id_Token_WithEndLocation
    /// @brief @brief Get the end location of this token.
    /// @return the end location of this token
    /// @see setEndLocation
    /// @remark The end location is the location at which the lexeme of this token ends at.
    const Location& getEndLocation() const
    {
        return m_endLocation;
    }

    /// @brief Set the end location of this token.
    /// @param endLocation the end location of this token
    /// @see getEndlocation
    void setEndLocation(const Location& endLocation)
    {
        m_endLocation = endLocation;
    }
#endif

	/// @brief @brief Get the start location of this token.
	/// @return the start location of this token
    /// @see setStartLocation
    /// @remark The start location is the location at which the lexeme of this token starts at.
	const Location& getStartLocation() const
	{
		return m_startLocation;
	}
	
	/// @brief Set the start location of this token.
	/// @param startLocation the start location of this token
    /// @see getStartLocation
	void setStartLocation(const Location& startLocation)
	{
		m_startLocation = startLocation;
	}

	/// @brief Get the lexeme of this token.
	/// @return the lexeme of this token
    /// @see setLexeme
	const std::string& getLexeme() const
	{
		return m_lexeme;
	}
	
	/// @brief Set the lexeme of this token.
	/// @param lexeme the lexeme of this token
    /// @see getLexeme
	void setLexeme(const std::string& lexeme)
	{
		m_lexeme = lexeme;
	}
	
public:
    /// @brief Get if this token is of the given kind.
    /// @param kind the kind
    /// @return @a true if this token is of the given kind @a kind, @a false otherwise
    bool is(const KindType& kind) const
	{
		return kind == getKind();
	}

    /// @brief Get if this token is of the given kinds.
    /// @param kind1, kind2 the kinds
    /// @return @a true if this token is of the kinds @a kind1 or @a kind2, @a false otherwise
    bool isOneOf(const KindType& kind1, const KindType& kind2) const
	{
		return is(kind1) || is(kind2);
	}

    /// @brief Get if this token is of the given kinds.
    /// @param kind1, kind2, kinds the types
    /// @return @a true if this token is of the given kinds @a kind1, @a kind2, or @a kinds, @a false otherwise
    template <typename ... Kinds>
    bool isOneOf(const KindType& kind1, const KindType& kind2, Kinds ... kinds) const
    {
        return is(kind1) || isOneOf(kind2, kinds ...);
    }

}; // struct Token

} // namespace Id
