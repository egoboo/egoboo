#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

template <typename Type, typename Enabled = void>
struct Interval;

/// @brief A single-precision floating-point interval specified by its lowerbound \f$l\f$ and its upperbound \f$l\f$ such that \f$l \leq u\f$.
template <typename Type>
struct Interval<Type, std::enable_if_t<std::is_floating_point<Type>::value>>
    : id::equal_to_expr<Interval<Type>> {
private:
    /// @brief The lowerbound (inclusive).
    Type l;
    /// @brief The upperbound (inclusive).
    Type u;

public:
    /// @brief Construct this range with default values.
    /// @remark The default values are the lowerbound of \f$0\f$ and the upperbound of \f$0\f$.
    Interval() :
        l(0.0f), u(0.0f) {}

    /// @brief Copy-construct this range from another range.
    /// @param other the other range
    Interval(const Interval<Type>& other) :
        l(other.l), u(other.u) {}

    /// @brief Construct this range with the specified values.
    /// @param l the lowerbound
    /// @param u the upperbound
    /// @pre <c>l &lt;= u</c>
    /// @throw Id::InvalidArgumentException <c>!(l &lt;= u)</c>
    Interval(const Type& l, const Type& u) :
        l(l), u(u) {
        if (!(l <= u)) {
            throw Id::InvalidArgumentException(__FILE__, __LINE__, "precondition `l <= u` failed");
        }
    }

public:
    /// @brief Assign this range with the values of another range.
    /// @param other the other range
    /// @return this range
    /// @post This range was assigned the values of the other range.
    const Interval<Type>& operator=(const Interval<Type>& other) {
        l = other.l;
        u = other.u;
        return *this;
    }

public:
    // CRTP
    bool equal_to(const Interval<Type>& other) const {
        return l == other.l
            && u == other.u;
    }

public:
    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) of this interval multiplied by a scalar.
    /// @param s the scalar
    /// @return the interval
    /// @remark Given an interval \f$a=[l,u]\f$ and a scalar \f$s\f$, this method computes a new interval \f$a' = s \cdot a = s \cdot [l,u] = [s\cdot l, s\cdot u]\f$.
    Interval<Type> operator*(const Type& s) const {
        return Interval<Type>(l * s, u * s);
    }

    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) of this interval multiplied by a scalar
    /// and assign the result to this interval.
    /// @param s the scalar
    /// @return the interval
    const Interval<Type>& operator*=(const Type& s) {
        (*this) = (*this) * s;
        return *this;
    }

public:
    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) of this interval divided by a scalar.
    /// @param s the scalar
    /// @return the interval
    /// @remark Given an interval \f$a=[l,u]\f$ and a scalar \f$s\f$, this method computes a new interval \f$a' = \frac{a}{s} = \frac{[l,u]}{s} = [\frac{l}{s}, \frac{u}{s}]\f$.
    Interval<Type> operator/(const Type& s) const {
        return Interval<Type>(l / s, u / s);
    }

    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) of this interval divided by a scalar
    /// and assign the result to this interval.
    /// @param s the scalar
    /// @return this interval
    const Interval<Type>& operator/=(const Type& s) {
        (*this) = (*this) / s;
        return *this;
    }

public:
    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) of this interval added by a scalar.
    /// @param s the scalar
    /// @return the interval
    /// @remark Given an interval \f$a=[l,u]\f$ and a scalar \f$s\f$, this method computes a new interval \f$a' = a + s = [l,u] + s = [l + s, u + s]\f$.
    Interval<Type> operator+(const Type& s) const {
        return Interval<Type>(l + s, u + s);
    }

    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) of this interval added by a scalar
    /// and assign the result to this range.
    /// @param s the scalar
    /// @return this interval
    const Interval<Type>& operator+=(const Type& s) {
        (*this) = (*this) + s;
        return *this;
    }

public:
    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) subtracted by a scalar.
    /// @param s the scalar
    /// @return the interval
    /// @remark Given an interval \f$a=[l,u]\f$ and a scalar \f$s\f$, this method computes a new interval \f$a' = a - s = [l,u] - s = [l - s, u - s]\f$.
    Interval<Type> operator-(const Type& s) const {
        return Interval<Type>(l - s, u - s);
    }

    /// @brief Compute a new interval with a lowerbound (upperbound) that is equal to the lowerbound (upperbound) of this interval subtracted by a scalar
    /// and assign the result to this range.
    /// @param s the scalar
    /// @return this interval
    const Interval<Type>& operator-=(const Type& s) {
        (*this) = (*this) - s;
        return *this;
    }

public:
    /// @brief Get the lowerbound.
    /// @return the lowerbound
    /// @invariant <c>getLowerbound() &lt;= getUpperbound()</c>
    const Type& getLowerbound() const {
        return l;
    }

    /// @brief Get the upperbound.
    /// @return the uppebound
    /// @invariant <c>getLowerbound() &lt;= getUpperbound()</c>
    const Type& getUpperbound() const {
        return u;
    }

public:
    /// @brief Get if this range is equivalent to the interval \f$[0,0]\f$.
    /// @return @a true if this range is equivalent to the interval \f$[0,0]\f$, @a false otherwise
    bool isZero() const {
        return std::abs(l) + std::abs(u) < std::numeric_limits<Type>::epsilon();
    }

};

} // namespace Math
} // namespace Ego
