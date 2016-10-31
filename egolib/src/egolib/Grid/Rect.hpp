#pragma once

#include "egolib/Grid/Index.hpp"

namespace Grid {

/// @todo Shall be renamed to <tt>IndexRange</tt> and shall have specializations
/// for all coordinate systems as well as support empty ranges and iterators.
template <typename UnderlayingType_, CoordinateSystem CoordinateSystem_>
struct IndexRectangle
{
public:
    using UnderlayingType = UnderlayingType_;
    using Index2Type = Index<UnderlayingType, CoordinateSystem::Grid>;
    using Index1Type = Index<UnderlayingType, CoordinateSystem::List>;
    using MyType = IndexRectangle<UnderlayingType, CoordinateSystem_>;

private:
    Index2Type _min;
    Index2Type _max;

public:
    IndexRectangle(const Index2Type& min, const Index2Type& max)
        : _min(min), _max(max)
    {}

    IndexRectangle(const MyType& other)
        : _min(other._min), _max(other._max)
    {}

public:
    const MyType& operator=(const MyType& other)
    {
        _min = other._min;
        _max = other._max;
        return *this;
    }

public:
    bool operator==(const MyType& other) const
    {
        return _min == other._min
            && _max == other._max;
    }

    bool operator!=(const MyType& other) const
    {
        return !(*this == other);
    }

public:
    Index2Type& min()
    {
        return _min;
    }

    const Index2Type& min() const
    {
        return _min;
    }

    Index2Type& max()
    {
        return _max;
    }

    const Index2Type& max() const
    {
        return _max;
    }
};

} // namespace Grid
