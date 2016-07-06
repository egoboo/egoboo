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
    : signal(nullptr), node(nullptr) {}

ConnectionBase::ConnectionBase(SignalBase *signal, NodeBase *node)
    : signal(signal), node(node) {}

ConnectionBase::ConnectionBase(const ConnectionBase& other)
    : signal(other.signal), node(other.node) {}

const ConnectionBase& ConnectionBase::operator=(const ConnectionBase& other) {
    signal = other.signal;
    node = other.node;
    return *this;
}

bool ConnectionBase::operator==(const ConnectionBase& other) const {
    return signal == other.signal
        && node == other.node;
}

bool ConnectionBase::operator!=(const ConnectionBase& other) const {
    return signal != other.signal
        || node != other.node;
}

bool ConnectionBase::isConnected() const {
    return nullptr != node;
}

void ConnectionBase::disconnect() {
    // If the connection is not connected ...
    if (node == nullptr || signal == nullptr) {
        // ... return immediatly.
        return;
    }
    // Mark the node as dead and increment the dead count and decrement the live count of this signal.
    if (node->kill()) {
        signal->deadCount++;
        signal->liveCount--;
    }
    // If this signal is not running ...
    if (!signal->running) {
        // ... mark it as running and sweep it.
        signal->running = true;
        signal->maybeSweep();
        signal->running = false;
    }
}
	
} // namespace Internal
} // namespace Ego
