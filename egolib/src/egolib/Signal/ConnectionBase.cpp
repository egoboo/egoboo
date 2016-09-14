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
        node->addReference();
    }
}

ConnectionBase::ConnectionBase(const ConnectionBase& other)
    : node(other.node) {
    if (node) {
        node->addReference();
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
            node->addReference();
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
        return NodeBase::State::Disconnected != node->state;
    }
    return false;
}

void ConnectionBase::reset() {
    if (nullptr == node) {
        return;
    }
    // Remove our reference to the node.
    node->removeReference();
    // If the number of references to the node is @a 0,
    // then we were the sole owner of this node.
    // We delete the node.
    if (0 == node->getNumberOfReferences()) {
        delete node;
        // If the number of references to the node is @a 1,
        // and the node has a signal, then the signal is the sole owner of this node.
        // If the node is disconnected, then the signal may want to delete the node.
    } else if (1 == node->getNumberOfReferences() && nullptr != node->signal && node->state == NodeBase::State::Disconnected) {
        // If this signal is not running ...
        if (!node->signal->running) {
            // ... mark it as running and sweep it.
            node->signal->running = true;
            node->signal->maybeSweep();
            node->signal->running = false;
        }
    // Otherwise some other connection owns the node now.
    }  else {
        /* Nothing to do. */
    }
    node = nullptr;
}

void ConnectionBase::disconnect() {
    if (node) {
        if (node->state != NodeBase::State::Disconnected) {
            node->state = NodeBase::State::Disconnected;
            SignalBase *signal = node->signal;
            if (signal) {
                // >> notify signal node disconnected
                signal->connectedCount--;
                signal->disconnectedCount++;
                // << notify signal node disconnected
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
	
} // namespace Internal
} // namespace Ego
