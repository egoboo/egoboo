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

#include "egolib/Signal/ConnectionBase.hpp"
#include "egolib/Signal/NodeBase.hpp"
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

 /// The opaque type of a connection.
struct Connection;

/// A node.
/// @todo Hide within Signal or within Internal namespace.
template <class>
struct Node;

template <class>
struct Signal;

/// A node.
/// @todo Hide within Signal or within Internal namespace.
template <class ReturnType, class ... ParameterTypes>
struct Node<ReturnType(ParameterTypes ...)> : Ego::Internal::NodeBase {
public:
    /// The node type.
    typedef Node<ReturnType(ParameterTypes ...)> NodeType;
    /// The function type.
    typedef std::function<ReturnType(ParameterTypes ...)> FunctionType;

public:
    /// The function.
    FunctionType function;

public:
    Node(const NodeType&) = delete; // Do not allow copying.
    const NodeType& operator=(const NodeType&) = delete; // Do not allow copying.

public:
    /// Construct this node.
    explicit Node(const FunctionType& function)
        : Ego::Internal::NodeBase(), function(function) {}

public:
    /// Invoke this node
    /// @param arguments (implied)
    /// @return (implied)
    /// @todo Is perfect forwarding required/desired?
    ReturnType operator()(ParameterTypes&& ... arguments) {
        function(std::forward<ParameterTypes>(arguments) ...);
    }
};

struct Connection : Ego::Internal::ConnectionBase {
    Connection()
        : Ego::Internal::ConnectionBase(nullptr) {}
    Connection(Ego::Internal::NodeBase *node)
        : Ego::Internal::ConnectionBase(node) {}
    Connection(const Connection& other)
        : Ego::Internal::ConnectionBase(other) {}
    const Connection& operator=(const Connection& other) {
        Ego::Internal::ConnectionBase::operator=(other);
        return *this;
    }
    bool operator==(const Connection& other) const {
        return ConnectionBase::operator==(other);
    }
    bool operator!=(const Connection& other) const {
        return ConnectionBase::operator!=(other);
    }
}; // struct Connection

/// @tparam ReturnType the return type
/// @tparam ... ParameterTypes the parameter types
/// @remark Non-copyable.
template <class ReturnType, class ... ParameterTypes>
struct Signal<ReturnType(ParameterTypes ...)> : Ego::Internal::SignalBase {
public:
    /// The node type.
    typedef Node<ReturnType(ParameterTypes ...)> NodeType;
    /// The function type.
    typedef std::function<ReturnType(ParameterTypes ...)> FunctionType;

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
    /// @return the subscription
    Connection subscribe(const FunctionType& function) {
        Ego::Internal::NodeBase *node = new NodeType(function);
        node->next = head; head = node;
        node->signal = this;
        liveCount++;
        return Connection(node);
    }


public:
    /// Notify all subscribers.
    /// @param arguments the arguments
    /// @todo Is perfect forwarding required/desired?
    void operator()(ParameterTypes&& ... arguments) {
        if (!running) { /// @todo Use ReentrantBarrier (not committed yet).
            running = true;
            try {
                for (Ego::Internal::NodeBase *cur = head; nullptr != cur; cur = cur->next) {
                    if (cur->hasConnections()) {
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
