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

SignalBase::~SignalBase() noexcept {
    while (nullptr != head) {
        NodeBase *node = head; head = node->next;
        delete node;
    }
    deadCount = 0;
    liveCount = 0;
}

bool SignalBase::needsSweep() const noexcept {
    return deadCount > std::min(size_t(8), liveCount);
}

void SignalBase::maybeSweep() noexcept {
    assert(true == running); // Must be true!
    if (needsSweep()) {
        NodeBase **pred = &head, *cur = head;
        while (nullptr != cur) {
            if (cur->isDead()) { // Does the subscription refer to this node?
                NodeBase *node = cur;
                *pred = cur->next;
                cur = cur->next;
                delete node;
                deadCount--;
            } else {
                pred = &cur->next;
                cur = cur->next;
            }
        }
    }
}

void SignalBase::unsubscribe(const ConnectionBase& connection) noexcept {
    // If the connection is not connected or not a connection to this signal ...
    if (connection.node == nullptr || connection.signal != this) {
        // ... return immediatly.
        return;
    }
    // Mark the node as dead and increment the dead count and decrement the live count of this signal.
    if (connection.node->kill()) {
        deadCount++;
        liveCount--;
    }
    // If this signal is not running ...
    if (!running) {
        // ... mark it as running and sweep it.
        running = true;
        maybeSweep();
        running = false;
    }
}


} // namespace Internal
} // namespace Ego
