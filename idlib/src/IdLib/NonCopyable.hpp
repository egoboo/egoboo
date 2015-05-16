#pragma once

/// @file   IdLib/NonCopyable.hpp
/// @brief  Make classes non-copyable.
/// @author Michael Heilmann

namespace Id
{
/**
 * @brief
 *  Inherit from this class to make the inheriting class and its descendant class non-copyable.
 *  Example usage
 *  @code
 *  class Foo : Bar, NonCopyable
 *  { ... }
 *  @endcode
 * @see http://en.cppreference.com/w/cpp/language/as_operator
 * @see http://en.cppreference.com/w/cpp/language/copy_constructor
 * @author
 *  Michael Heilmann
 */
class NonCopyable
{

protected:
    NonCopyable() { }
    ~NonCopyable() { }
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace Id
