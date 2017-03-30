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
    /// @brief The type of an index into a constant pool.
    using Index = uint32_t;

    /// @brief The type of the size of a constant pool.
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

    ConstantPool& operator=(const ConstantPool& other);
    /// @brief Move-assign this constant pool with the values of another constant pool.
    /// @param other the other constant pool
    /// @return a constant reference to this constant pool
    /// @post The target was assigned the values of the source. The source is empty.
    ConstantPool& operator=(const ConstantPool&& other);

    /**@}*/

public:
    /// @brief Get the constant at the specified index.
    /// @param index the index
    /// @return a reference to the constant
    /// @throw id::runtime_error the index was out of bounds
    const Constant& getConstant(Index index);

    /// @brief Get the number of constants.
    /// @return the number of constants
    Size getNumberOfConstants() const;

    /// @brief Get or create an <c>integer</c> constant.
    /// @param value the value of the constant
    /// @return the index of the constant
    /// @throw id::runtime_error the constant pool would overflow
    Index getOrCreateConstant(int value);

    /// @brief Get or create a <c>string</c> constant.
    /// @param value the value of the constant.
    /// @return the index of the constant
    /// @throw id::runtime_error the constant pool would overflow
    Index getOrCreateConstant(const std::string& value);

    /// @brief Clear this constant pool.
    void clear();
};

} // namespace Script
} // namespace Ego
