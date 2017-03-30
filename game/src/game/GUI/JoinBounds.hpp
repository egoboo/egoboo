#pragma once

#include "game/GUI/Component.hpp"

namespace Ego {
namespace GUI {

/// Computes the bounds of list \f$C\f$ of components with \f$|C|>0\f$.
/// This functor returns \f$\bigcup_{c \in C} b(c)\f$ where \f$b(c)\f$ are the bounds of the component \f$c\f$.
struct JoinBounds {
    template <typename InputIteratorType>
    Rectangle2f compute(InputIteratorType start, InputIteratorType end) const {
        if (start == end) {
            throw id::invalid_argument_error(__FILE__, __LINE__, "empty component list");
        }
        auto current = start;
        auto bounds = (*current)->getBounds();
        current++;
        while (current != end) {
            bounds.join((*current)->getBounds());
            current++;
        }
        return bounds;
    }
    /// Returns the bounds of the components of the specified container.
    Rectangle2f operator()(Container& container) const {
        auto it = container.iterator();
        return compute(it.cbegin(), it.cend());
    }
    /// Returns the bounds of the components of the specified component list.
    /// @undefined the components must be children of the same container
    template <typename InputIteratorType>
    Rectangle2f operator()(InputIteratorType start, InputIteratorType end) const {
        return compute(start, end);
    }
    Rectangle2f operator()(std::initializer_list<std::shared_ptr<Component>> list) const {
        return compute(list.begin(), list.end());
    }
};

} // namespace GUI
} // namespace Ego
