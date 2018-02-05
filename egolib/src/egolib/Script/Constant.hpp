#pragma once

#include "egolib/platform.h"

namespace Ego
{
namespace Script
{

/// @brief A constant in the in-memory representation of a compiled script.	
struct Constant
{
public:
	/// @brief The kind of constants.
    enum class Kind
    {
        Void = 0, ///< @brief A constant of type <c>void</c>.
        Integer,  ///< @brief A constant of type <c>integer</c>.
        String,   ///< @brief A constant of type <c>string</c>.
    };

private:
    Kind m_kind; ///< @brief The kind of this constant.
    union
    {
        int m_integer; ///< @brief The value of this constant. Only valid if m_kind is Kind::Integer.
        std::string m_string; ///< @brief The value of this constant. Only valid if m_kind is Kind::String.
    };
public:
	/// @brief Construct a constant of type Kind::Void.
    Constant();

    /// @brief Construct this constant with the values of another constant.
    /// @param other the other constant
    Constant(const Constant& other);
    Constant(Constant&& other);

	/// @brief Construct a constant of kind Kind::Integer and the specified integer value.
	/// @param value the value
    explicit Constant(int value);

    /// @brief Construct a constant of kind Kind::String and the specified string value.
    /// @param value the value
    explicit Constant(const std::string& value);

    /// @brief Destruct this constant.
    ~Constant();

	/// @brief Assign this constant with the values of another constant.
	/// @param other the other constant
	/// @return this constant
    /// @remark If an exception is raised, the state of the target constant was not modified.
    Constant& operator=(const Constant& other);

	/// @brief Get if this constant is equal to another constant.
	/// @param other the other constant
	/// @return @a true if this constant is equal to the other constant, @a false otherwise
	/// @remark Two constants @a x and @a y are not equal if they have different kinds.
	/// If @a x and @a y have the same kind and that kind is Kind::Void, then their are equal.
	/// If @a x and @a y have the same kind and that kind is not Kind::Void, they are equal if their values are equal.
    bool operator==(const Constant& other) const;

public:
    Kind getKind() const
    {
        return m_kind;
    }

    int getAsInteger() const
    {
        if (Constant::Kind::Integer != m_kind)
        {
            throw idlib::runtime_error(__FILE__, __LINE__, "invalid conversion");
        }
        return m_integer;
    }

    const std::string& getAsString() const
    {
        if (Kind::String != m_kind)
        {
            throw idlib::runtime_error(__FILE__, __LINE__, "invalid conversion");
        }
        return m_string;
    }
};
	
} // namespace Script
} // namespace Ego
