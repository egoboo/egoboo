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
        Integer,  ///< @brief A constant of tyoe <c>integer</c>.
    };

private:
    Kind m_kind; ///< @brief The kind of this constant.
    int m_integer; ///< @brief The value of this constant.

public:
	/// @brief Construct a constant of type Kind::Unknown.
    Constant();

    /// @brief Construct this constant with the values of another constant.
    /// @param other the other constant
    Constant(const Constant& other);
    Constant(Constant&& other);

	/// @brief Construct a constant of type Kind::Integer and the specified integer value.
	/// @param value the value
    Constant(int value);

	/// @brief Assign this constant with the values of another constant.
	/// @param other the other constant
	/// @return this constant
    Constant& operator=(Constant other);

	/// @brief Get if this constant is equal to another constant.
	/// @param other the other constant
	/// @return @a true if this constant is equal to the other constant, @a false otherwise
	/// @remark Two constants @a x and @a y are not equal if they have different kinds.
	/// If @a x and @a y have the same kind and that kind is Kind::Void, then their are equal.
	/// If @a x and @a y have the same kind and that kind is not Kind::Void, they are equal if their values are equal.
    bool operator==(const Constant& other) const;

public:
    int getAsInteger() const
    {
        if (Constant::Kind::Integer != m_kind)
        {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "invalid conversion");
        }
        return m_integer;
    }


public:
    friend void swap(Constant& x, Constant& y)
    {
        using std::swap;

        if (&x == &y)
        {
            return;
        }
        swap(x.m_kind, y.m_kind);
        switch (x.m_kind)
        {
            case Constant::Kind::Integer:
                swap(x.m_integer, y.m_integer);
                break;
            case Constant::Kind::Void:
                break;
        }

    }
};
	
} // namespace Script
} // namespace Ego
