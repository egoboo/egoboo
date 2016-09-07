#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {


/**
 * @brief The Generator template defines a function object that implements
 * a generator function. Instances of this function object satisfy the
 * the Generator concept. In particular, they define an <tt>operator()</tt>
 * that
 * - accepts a single parameter of type <tt>size_t</tt>
 * - Returns a value of type <tt>_ReturnValueType</tt> that is a function of the parameter
 *
 * The hash template is
 * <a href="http://en.cppreference.com/w/cpp/concept/DefaultConstructible">DefaultConstructible</a>,
 * <a href="http://en.cppreference.com/w/cpp/concept/CopyAssignable">CopyAssignable</a>,
 * <a href="http://en.cppreference.com/w/cpp/concept/Swappable">Swappable</a> and
 * <a href="http://en.cppreference.com/w/cpp/concept/Destructible">Destructible</a>.
 */
template <typename _ReturnValueType>
struct Generator {};

/// A generator which takes an index as an argument and returns a constant value.
/// @remark A generator implements <tt>operator() const</tt> which takes an index
/// as its single argument and returns a value of type <tt>_ResultType</tt>.  The
/// value is a function of the index.
template <typename _ResultType>
struct ConstantGenerator : Generator<_ResultType> {
public:
    using ResultType = _ResultType;
private:
    const ResultType constantValue;
public:
    ConstantGenerator(const ResultType& constantValue = ResultType())
        : constantValue(constantValue) {
    }
public:
#if defined(_MSC_VER) // Disable sickening flood of warnings.
    #pragma warning(push)
    #pragma warning(disable: 4100)
#endif
    ResultType operator()(size_t index) const {
        return constantValue;
    }
#if defined(_MSC_VER)
    #pragma warning(pop)
#endif
};

/// A generator implementing an <tt>operator()</tt> which takes an index as its single argument.
/// If that index is equal to a specified index then one constant value is returned otherwise
/// another constant value is returned.
template <typename _ResultType>
struct ConditionalGenerator : Generator<_ResultType> {
public:
    using ResultType = _ResultType;
private:
    const size_t indexIfTrue;
    const ResultType constantValueIfTrue;
    const ResultType constantValueIfFalse;
public:
    ConditionalGenerator(size_t indexIfTrue = size_t(), const ResultType& constantValueIfTrue = ResultType(),
                         const ResultType& constantValueIfFalse = ResultType())
        : indexIfTrue(indexIfTrue), constantValueIfTrue(constantValueIfTrue), constantValueIfFalse(constantValueIfFalse) {
    }
public:
    ResultType operator()(size_t index) const {
        return index == indexIfTrue ? constantValueIfTrue : constantValueIfFalse;
    }
};

} // namespace Math
} // namespace Ego
