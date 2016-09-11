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

SignalBase::SignalBase() noexcept : head(nullptr), disconnectedCount(0), connectedCount(0), running(false) {}

void SignalBase::sweep() noexcept {
    NodeBase **predecessor = &head, *current = head;
    while (nullptr != current) {
        // Remove disconnected nodes.
        if (current->isDisconnected()) {
            // Unlink the node.
            NodeBase *node = current;
            *predecessor = current->next; // Let predecessor refer to the successor.
            current = current->next; // Continue at successor.
            // Decrement the number of disconnected nodes.
            disconnectedCount--;
            // Remove the reference from this signal.
            node->removeReference();
            // If the number of references to the node is @a 0, then the signal was the sole owner of the node.
            // The signal shall delete the node.
            if (0 == node->getNumberOfReferences()) {
                delete node;
            }
        } else {
            predecessor = &current->next;
            current = current->next;
        }
    }
}

SignalBase::~SignalBase() noexcept {
    disconnectAll();
    sweep();
    assert(0 == connectedCount);
    assert(0 == disconnectedCount);
    assert(nullptr == head);

}

bool SignalBase::needsSweep() const noexcept {
    return disconnectedCount > std::min(size_t(8), connectedCount);
}

void SignalBase::maybeSweep() noexcept {
    assert(true == running); // Must be true!
    if (needsSweep()) {
        sweep();
    }
}

void SignalBase::disconnectAll() noexcept {
    for (auto node = head; nullptr != node; node = node->next) {
        if (node->state != Ego::Internal::NodeBase::State::Disconnected) {
            node->state = Ego::Internal::NodeBase::State::Disconnected;
            disconnectedCount++;
            connectedCount--;
        }
    }
}

} // namespace Internal
} // namespace Ego
