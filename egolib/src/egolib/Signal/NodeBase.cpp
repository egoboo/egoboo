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

/// @file egolib/Signal/NodeBase.cpp
/// @brief Non-generic base class of all nodes.

#include "egolib/Signal/NodeBase.hpp"

namespace Ego {
namespace Internal {

NodeBase::NodeBase()
    : signal(nullptr), numberOfConnections(0), next(nullptr)
#if 0
    , dead(false) 
#endif
{}

NodeBase::~NodeBase() {}

#if 0
bool NodeBase::kill() noexcept {
    bool temporary = dead;
    dead = true;
    return !temporary;
}
#endif

#if 0
bool NodeBase::isDead() const noexcept {
    return dead;
}
#endif

bool NodeBase::hasSignal() const {
    return nullptr != signal;
}

bool NodeBase::hasConnections() const {
    return 0 < getNumberOfConnections();
}

int NodeBase::getNumberOfConnections() const {
    return numberOfConnections;
}

void NodeBase::onConnectionAdded() {
    numberOfConnections++;
}

void NodeBase::onConnectionRemoved() {
    if (0 == --numberOfConnections) {
        /* Nothing to do yet. */
    }
}

} // namespace Internal
} // namespace Ego
