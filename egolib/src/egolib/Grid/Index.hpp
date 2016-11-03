#pragma once

#include "egolib/typedef.h"

namespace Grid {

/// @brief An enumeration of coordinate system used by/for/together with meshes.
enum class CoordinateSystem
{
    /// @brief "tile grid"/"2D" coordinates.
    Grid,
    /// @brief "tile list"/"1D" coordinates.
    List,
};

/// @brief An index "tile grid"/"2D" or "tile list"/"1D" coordinates.
/// @tparam UnderlayingType_ the underlaying index type
/// @tparam CoordinateSystem_ the coordinate system
/// @remark Specializations for
/// <tt>UnderlayingType_ = int</tt> and
/// all coordinate systems
/// are provided.
template <typename UnderlayingType_, CoordinateSystem CoordinateSystem_, typename Enabled_ = void>
struct Index;

namespace Internal {

template <typename UnderlayingType_, CoordinateSystem CoordinateSystem_>
using EnableIndex2 = std::enable_if_t<CoordinateSystem_ == CoordinateSystem::Grid && std::is_same<UnderlayingType_, int>::value>;

} // namespace Internal

/// @brief A "tile grid"/"2D" index.
template <typename UnderlayingType_, CoordinateSystem CoordinateSystem_>
struct Index<UnderlayingType_, CoordinateSystem_, Internal::EnableIndex2<UnderlayingType_, CoordinateSystem_>>
    : public Id::EqualToExpr<Index<UnderlayingType_, CoordinateSystem_>>
{
public:
    using UnderlayingType = UnderlayingType_;
    using MyType = Index<UnderlayingType, CoordinateSystem_>;
private:
    UnderlayingType _x; ///< @brief The x-coordinate.
    UnderlayingType _y; ///< @brief The y-coordinate.

public:
    Index(const UnderlayingType& x, const UnderlayingType& y) :
        _x(x), _y(y)
    {}

public:
    MyType& operator=(const MyType& other)
    {
        _x = other._x;
        _y = other._y;
        return *this;
    }

public:
	// CRTP
    bool equalTo(const MyType& other) const
    {
        return _x == other._x
            && _y == other._y;
    }

public:
    UnderlayingType& x()
    {
        return _x;
    }

    const UnderlayingType& x() const
    {
        return _x;
    }

    UnderlayingType& y()
    {
        return _y;
    }

    const UnderlayingType& y() const
    {
        return _y;
    }

};

namespace Internal {

template <typename UnderlayingType_, CoordinateSystem CoordinateSystem_>
using EnableIndex1 = std::enable_if_t<CoordinateSystem_ == CoordinateSystem::List && std::is_same<UnderlayingType_, int>::value>;

} // namespace Internal

/// @brief A "list"/"1D" index.
template <typename UnderlayingType_, CoordinateSystem CoordinateSystem_>
struct Index<UnderlayingType_, CoordinateSystem_, Internal::EnableIndex1<UnderlayingType_, CoordinateSystem_>>
    : public Id::EqualToExpr<Index<UnderlayingType_, CoordinateSystem_>>,
      public Id::LowerThanExpr<Index<UnderlayingType_, CoordinateSystem_>>,
      public Id::IncrementExpr<Index<UnderlayingType_, CoordinateSystem_>>
{
public:
    using UnderlayingType = UnderlayingType_;
    using MyType = Index<UnderlayingType, CoordinateSystem_>;

private:

    UnderlayingType _i; ///< @brief The i-coordinate.

    static const UnderlayingType _InvalidIndex = std::numeric_limits<UnderlayingType>::max();

public:
    static const MyType Invalid;

    Index() :
        _i(_InvalidIndex)
    {}

    Index(const UnderlayingType& i) :
        _i(i)
    {}

public:

    Index(const MyType& other) :
        _i(other._i)
    {}

    MyType& operator=(const MyType& other)
    {
        _i = other._i;
        return *this;
    }

public:
	// CRTP
    bool equalTo(const MyType& other) const
    {
        return _i == other._i;
    }

	// CRTP
    bool lowerThan(const MyType& other) const
    {
        return _i < other._i;
    }

	// CRTP
    void increment()
    {
        _i++;
    }

public:

    UnderlayingType& i()
    {
        return _i;
    }

    const UnderlayingType& i() const
    {
        return _i;
    }
};

template <typename UnderlayingType_, CoordinateSystem CoordinateSystem_>
const Index<UnderlayingType_, CoordinateSystem_>
Index<UnderlayingType_, CoordinateSystem_, Internal::EnableIndex1<UnderlayingType_, CoordinateSystem_>>::Invalid;

template <typename UnderlayingType>
Index<UnderlayingType, CoordinateSystem::List> map(Index<UnderlayingType, CoordinateSystem::Grid> source, UnderlayingType width)
{
    // Assume a grid of 3 x 3 elements as displayed below:
    // 0  1  2
    // 3  4  5
    // 6  7  8
    // 9 10 11
    // The grid index row y = 3 and the column x = 1 should map to list index i = 10.
    // Using the formula we obtain i = 3 * 3 + 1 = 10 as desired.
    return Index<UnderlayingType, CoordinateSystem::List>(source.x() + source.y() * width);
}

template <typename UnderlayingType>
Index<UnderlayingType, CoordinateSystem::Grid> map(Index<UnderlayingType, CoordinateSystem::List> source, UnderlayingType width)
{
    // Assume a grid of 3 x 3 elements as displayed below:
    // 0  1  2
    // 3  4  5
    // 6  7  8
    // 9 10 11
    // The index i = 10 should map to row y = 3 and to column x = 1.
    // Using the formula we obtain x = 10 % 3 = 1 and y = 10 / 3 = 3 as desired.
    return Index<UnderlayingType, CoordinateSystem::Grid>(source.i() % width, source.i() / width);
}

} // namespace Grid
