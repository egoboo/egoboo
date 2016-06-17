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

#include "egolib/platform.h"

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

 /// The opaque type of a subscription.
struct Subscription;

/// A node.
/// @todo Hide within Signal or within Internal namespace.
template <class>
struct Node;

/// A node.
/// @todo Hide within Signal or within Internal namespace.
template <class ReturnType, class ... ParameterTypes>
struct Node<ReturnType(ParameterTypes ...)> {
public:
    /// The node type.
    typedef Node<ReturnType(ParameterTypes ...)> NodeType;
    /// The function type.
    typedef std::function<ReturnType(ParameterTypes ...)> FunctionType;

public:
    /// A pointer to the successor   of this node if it has a successor,   a null pointer otherwise.
    NodeType *next;
    /// Is this node dead?
    bool dead;
    /// The function.
    FunctionType function;

public:
    Node(const NodeType&) = delete; // Do not allow copying.
    const NodeType& operator=(const NodeType&) = delete; // Do not allow copying.

public:
    /// Construct this node.
    explicit Node(const FunctionType& function)
        : next(nullptr), dead(false), function(function) {}

public:
    /// Invoke this node
    /// @param arguments (implied)
    /// @return (implied)
    /// @todo Is perfect forwarding required/desired?
    ReturnType operator()(ParameterTypes&& ... arguments) {
        function(std::forward<ParameterTypes>(arguments) ...);
    }

public:
    /// Kill this node.
    /// @postcondition this node is dead
    /// @return @a true if the node was killed by this call, @a false otherwise
    bool kill() noexcept { bool temporary = dead; dead = true; return !temporary; }

    /// Get if this node is dead.
    /// @return @a true if this node is dead, @a false otherwise
    bool isDead() const noexcept { return dead; }

};

template <class>
struct Signal;

struct Subscription {
    void *ptr;
    Subscription()
        : ptr(nullptr) {}
    Subscription(void *ptr)
        : ptr(ptr) {}
    Subscription(const Subscription& other)
        : ptr(other.ptr) {}
    const Subscription& operator=(const Subscription& other) {
        ptr = other.ptr;
        return *this;
    }
    bool operator==(const Subscription& other) const {
        return ptr == other.ptr;
    }
    bool operator!=(const Subscription& other) const {
        return ptr != other.ptr;
    }
    operator bool() const {
        return nullptr != ptr;
    }
}; // struct Subscription

/// @tparam ReturnType the return type
/// @tparam ... ParameterTypes the parameter types
/// @remark Non-copyable.
template <class ReturnType, class ... ParameterTypes>
struct Signal<ReturnType(ParameterTypes ...)> {
public:
    /// The node type.
    typedef Node<ReturnType(ParameterTypes ...)> NodeType;
    /// The function type.
    typedef std::function<ReturnType(ParameterTypes ...)> FunctionType;

public:
    Signal(const Signal&) = delete; // Do not allow copying.
    const Signal& operator=(const Signal&) = delete; // Do not allow copying.

private:
    NodeType *head; ///< The head.
    bool running;   ///< @a true if the signal is currently running, @a false otherwise
    size_t deadCount; ///< The number of dead nodes.
    size_t liveCount; ///< The number of live nodes.

public:
    /// Construct this signal.
    Signal() noexcept : head(nullptr), deadCount(0), liveCount(0), running(false) {}

    /// Destruct this signal
    /// (disconnecting all subscribers).
    ~Signal() noexcept {
        while (nullptr != head) {
            NodeType *node = head; head = node->next;
            delete node;
        }
        deadCount = 0;
        liveCount = 0;
    }

public:
    /// Subscribe to this signal.
    /// @param function a non-empty function
    /// @return the subscription
    Subscription subscribe(const FunctionType& function) {
        NodeType *node = new NodeType(function);
        node->next = head; head = node;
        liveCount++;
        return Subscription(static_cast<void *>(node));
    }

    /// Does the subscriber list need sweeping?
    /// @return @a true if the subscriber list needs sweeping, @a false otherwise
    bool needsSweep() const noexcept {
        return deadCount > std::min(size_t(8), liveCount);
    }

    /// Remove all dead subscriptions if the number of dead nodes exceeds the number of live nodes.
    /// @precondition The signal is not currently running.
    /// @todo Make private.
    void maybeSweep() noexcept {
        assert(true == running); // Must be true!
        if (needsSweep()) {
            NodeType **pred = &head, *cur = head;
            while (nullptr != cur) {
                if (cur->isDead()) { // Does the subscription refer to this node?
                    NodeType *node = cur;
                    *pred = cur->next;
                    cur = cur = cur->next;
                    delete node;
                    deadCount--;
                } else {
                    pred = &cur->next;
                    cur = cur->next;
                }
            }
        }
    }

    /// Unsubscribe from this signal.
    /// @param subscription the subscription
    /// @return @a true if the subscription was found and removed, @a false otherwise
    void unsubscribe(const Subscription& subscription) noexcept {
        if (!subscription) {
            return;
        }
        if (static_cast<NodeType *>(subscription.ptr)->kill()) {
            deadCount++;
            liveCount--;
        }
        if (!running) {
            running = true;
            maybeSweep();
            running = false;
        }
    }

public:
    /// Notify all subscribers.
    /// @param arguments the arguments
    /// @todo Is perfect forwarding required/desired?
    void operator()(ParameterTypes&& ... arguments) {
        if (!running) { /// @todo Use ReentrantBarrier (not committed yet).
            running = true;
            try {
                for (NodeType *cur = head; nullptr != cur; cur = cur->next) {
                    if (!cur->isDead()) {
                        (*cur)(std::forward<ParameterTypes>(arguments) ...);
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
