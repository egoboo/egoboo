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

/// @file egolib/Signal/NodeBase.hpp
/// @brief Non-generic base class of all nodes.

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Internal {

// Forward declarations.
struct NodeBase;
struct SignalBase;

/// Non-generic base class of any node.
struct NodeBase {
    /// The state of the relation of a signal and a slot.
    enum class State {
        Connected,
        Disconnected,
    };

    /// A pointer to the signal.
    SignalBase *signal;
    /// A pointer to the successor of this node if it has a successor, a null pointer otherwise.
    NodeBase *next;
    /// The state of the relation of the signal and the slot.
    State state;

    NodeBase(const NodeBase&) = delete; // Do not allow copying.
    const NodeBase& operator=(const NodeBase&) = delete; // Do not allow copying.

    /// @brief Construct this node.
    /// @param numberOfReferences the initial number of references of this node
    /// @post signal = nullptr, next = nullptr, state = State::Disconnected
    NodeBase(int numberOfReferences);

    /// @brief Virtual destructor.
    virtual ~NodeBase();

public:
    /// @brief Get if this node has a signal.
    /// @return @a true if this node has a signal, @a false otherwise
    bool hasSignal() const;

public:
    /// @brief Ensure that the state of this node is "disconnected".
    /// @post !o.isConnected()
    void disconnect() {
        state = NodeBase::State::Disconnected;
    }
    /// @brief Get if the state of this node is "connected".
    /// @return @a true if the state of this node is "connected", @a false otherwise
    bool isConnected() const {
        return NodeBase::State::Connected == state;
    }
    /// @brief Get if the state of this node is "disconnected".
    /// @return @a true if the state of this node is "disconnected", @a false otherwise
    bool isDisconnected() const {
        return !isConnected();
    }
public:
    int numberOfReferences;

public:
    /// @brief Get if this node has references.
    /// @return @a true if this node has references, @a false otherwise
    bool hasReferences() const;
    
    /// @brief The number of references to this node.
    int numberOfConnections;
    
    /// @brief Get the number of references of this node.
    /// @return the number of references of this node
    int getNumberOfReferences() const;

    /// @brief Add a reference to this node.
    void addReference();

    /// @brief Remove a reference to this node.
    void removeReference();
};

} // namespace Internal
} // namespace Ego
