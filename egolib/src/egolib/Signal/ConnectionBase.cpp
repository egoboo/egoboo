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

/// @file egolib/Signal/ConnectionBase.cpp
/// @brief Non-generic base class of all connections.

#include "egolib/Signal/ConnectionBase.hpp"
#include "egolib/Signal/SignalBase.hpp"
#include "egolib/Signal/NodeBase.hpp"

namespace Ego {
namespace Internal {

ConnectionBase::ConnectionBase()
    : node(nullptr) {}

ConnectionBase::ConnectionBase(NodeBase *node)
    : node(node) {
    if (node) {
        node->onConnectionAdded();
    }
}

ConnectionBase::ConnectionBase(const ConnectionBase& other)
    : node(other.node) {
    if (node) {
        node->onConnectionAdded();
    }
}

ConnectionBase::~ConnectionBase() {
    reset();
}

const ConnectionBase& ConnectionBase::operator=(const ConnectionBase& other) {
    if (&other != this) {
        reset();
        node = other.node;
        if (node) {
            node->onConnectionAdded();
        }
    }
    return *this;
}

bool ConnectionBase::operator==(const ConnectionBase& other) const {
    return node == other.node;
}

bool ConnectionBase::operator!=(const ConnectionBase& other) const {
    return node != other.node;
}

bool ConnectionBase::isConnected() const {
    if (node) {
        return nullptr != node->signal;
    }
    return false;
}

void ConnectionBase::reset() {
    if (nullptr != node) {
        // Remove the connection from this node.
        node->onConnectionRemoved();
        if (0 == node->getNumberOfConnections()) {
            // If the node has no signal, then it is our duty to delete the node.
            if (nullptr == node->signal) {
                delete node;
            // Otherwise notify the signal that one of its nodes has no connections anymore.
            } else {
                SignalBase *signal = node->signal;
                signal->deadCount++;
                signal->liveCount--;
                // If this signal is not running ...
                if (!signal->running) {
                    // ... mark it as running and sweep it.
                    signal->running = true;
                    signal->maybeSweep();
                    signal->running = false;
                }
            }
        }
        node = nullptr;
    }
}

void ConnectionBase::disconnect() {
    reset();
}
	
} // namespace Internal
} // namespace Ego
