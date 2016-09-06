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

/// @file egolib/Signal/SignalBase.hpp
/// @brief Non-generic base class of all signals.

#include "egolib/Signal/SignalBase.hpp"
#include "egolib/Signal/ConnectionBase.hpp"
#include "egolib/Signal/NodeBase.hpp"

namespace Ego {
namespace Internal {

SignalBase::SignalBase() noexcept : head(nullptr), deadCount(0), liveCount(0), running(false) {}

void SignalBase::sweep() noexcept {
    NodeBase **predecessor = &head, *current = head;
    while (nullptr != current) {
        // If there are no connections to this node it is the responsibility of the signal to deallocate the node:
        if (0 == current->getNumberOfConnections()) {
            // Unlink the node.
            NodeBase *node = current;
            *predecessor = current->next; // Let predecessor refer to the successor.
            current = current->next; // Continue at successor.
            deadCount--; // Decrement the number of dead nodes.
            delete node;
        } else {
            predecessor = &current->next;
            current = current->next;
        }
    }
}

SignalBase::~SignalBase() noexcept {
    sweep();
    assert(0 == deadCount);
    while (nullptr != head) {
        NodeBase *node = head; head = head->next;
        // Indicate that the node has no signal associated.
        node->signal = nullptr;
        node->next = nullptr;
    }
    liveCount = 0;
}

bool SignalBase::needsSweep() const noexcept {
    return deadCount > std::min(size_t(8), liveCount);
}

void SignalBase::maybeSweep() noexcept {
    assert(true == running); // Must be true!
    if (needsSweep()) {
        sweep();
    }
}

void SignalBase::unsubscribe(ConnectionBase& connection) noexcept {
    // Disconnect the connection.
    connection.disconnect();
}


} // namespace Internal
} // namespace Ego
