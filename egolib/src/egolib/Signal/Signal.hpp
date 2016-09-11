//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/Signal/Signal.hpp
/// @detail Signal-slot implementation.

#pragma once

#include "egolib/Signal/Connection.hpp"
#include "egolib/Signal/Node.hpp"
#include "egolib/Signal/SignalBase.hpp"

/**
 *  @defgroup ego-signal
 *  @brief C++ 11 signal-slot library.
 * Features:
 * - signals support arbitrary argument lists
 * - signals need to provided single-thread re-entrancy i.e. it is possible to connect and disconnect
 *   signal handlers and re-emit a signal from a running signal handler.
 * - signals support void return values
 * - a signal member in a user-defined type has moderate impact
 * -- on the cost of a call to a constructor/destructor of that type and
 *   -- on the size of an object of that type.
 */

namespace Ego {
/**
 * @addtogroup ego-signal
 * @{
 */

// Forward declarations.
struct Connection;
template <class> struct Signal;

/// @tparam ReturnType the return type
/// @tparam ... ParameterTypes the parameter types
/// @remark Non-copyable.
template <class ReturnType, class ... ParameterTypes>
struct Signal<ReturnType(ParameterTypes ...)> : Ego::Internal::SignalBase {
public:
    /// The node type.
    using NodeType = Ego::Internal::Node<ReturnType(ParameterTypes ...)>;
    /// The function type.
    using FunctionType = std::function<ReturnType(ParameterTypes ...)>;

public:
    Signal(const Signal&) = delete; // Do not allow copying.
    const Signal& operator=(const Signal&) = delete; // Do not allow copying.

public:
    /// Construct this signal.
    Signal() noexcept : SignalBase() {}

    /// Destruct this signal
    /// Disconnects all subscribers.
    ~Signal() noexcept {}

public:
    /// Subscribe to this signal.
    /// @param function a non-empty function
    /// @return the connection
    Connection subscribe(const FunctionType& function) {
        // Create the node.
        Ego::Internal::NodeBase *node = new NodeType(1, function);
        // Configure and add the node.
        node->state = Ego::Internal::NodeBase::State::Connected;
        node->next = head; head = node;
        node->signal = this;
        // Increment the connected count.
        connectedCount++;
        // Return the connection.
        return Connection(node);
    }


public:
    /// Notify all subscribers.
    /// @param arguments the arguments
    /// @remark
    /// Iterate over the nodes. If a node is connected, then it is invoked.
    void operator()(ParameterTypes&& ... arguments) {
        if (!running) { /// @todo Use ReentrantBarrier (not committed yet).
            running = true;
            try {
                for (Ego::Internal::NodeBase *cur = head; nullptr != cur; cur = cur->next) {
                    if (cur->state == Ego::Internal::NodeBase::State::Connected) {
                        (*static_cast<NodeType *>(cur))(std::forward<ParameterTypes>(arguments) ...);
                    }
                }
            } catch (...) {
                running = false;
                std::rethrow_exception(std::current_exception());
            }
            maybeSweep();
            running = false;
        }
    }

}; // struct Signal

/**@}*/
} // namespace Ego
