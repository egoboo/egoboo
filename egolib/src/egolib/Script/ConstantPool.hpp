#pragma once

#include "egolib/Script/Constant.hpp"

namespace Ego {
namespace Script {

/// @brief A constant pool in the in-memory representation of a compiled script.
/// A constant pool is a set of constants. Constants can only be added, they can
/// not be removed. When a constant is added it is assigned an index by which it.
/// Addition is O(n) in worst case, access is O(1) in worst case.
struct ConstantPool
{
protected:
    /// @brief Ordered set of constants.
    std::vector<Constant> m_constants;

public:
    /// @brief An index into a constant pool.
    using Index = uint32_t;

    /// @brief A size of a constant pool.
    using Size = uint32_t;

public:
    /// @brief Construct this constant pool.
    /// @post This constant pool is empty.
    ConstantPool();

    /**@{*/

    /// @brief Construct this constant pool with the values of another constant pool.
    /// @param other the other constant pool
    /// @post This constant pool has the same values as the other constant pool.
    /// @remark Classic rule of four copy-constructor.
    ConstantPool(const ConstantPool& other);

    ConstantPool(ConstantPool&& other);

    /**@}*/

    /**@{*/

    /// @brief Assign this constant pool with the values of another constant pool.
    /// @param other the other constant pool
    /// @return a constant reference to this constant pool
    /// @post This constant pool has the same values as the other constant pool.
    /// @remark Rule of 5 alternative assignment operator.
    ConstantPool& operator=(ConstantPool other);

    /**@}*/

public:
    /// @brief Get the constant at the specified index.
    /// @param index the index
    /// @return a reference to the constant
    /// @throw Id::RuntimeErrorException the index was out of bounds
    const Constant& getConstant(Index index);

    /// @brief Get the number of constants.
    /// @return the number of constants
    Size getNumberOfConstants() const;

    /// @brief Get or create an <c>integer</c> constant.
    /// @param value the constant value
    /// @return the index of the constant
    /// @throw Id::RuntimeErrorException the constant pool overflowed
    Index getOrCreateConstant(int value);

    /// @brief Clear this constant pool.
    void clear();

public:
    friend void swap(ConstantPool& x, ConstantPool& y)
    {
        std::swap(x.m_constants, y.m_constants);
    }
};

} // namespace Script
} // namespace Ego
