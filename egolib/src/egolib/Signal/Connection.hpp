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

/// @file egolib/Signal/Connection.hpp
/// @brief A connection.

#pragma once

#include "egolib/Signal/ConnectionBase.hpp"
#include "egolib/Signal/NodeBase.hpp"
#include "egolib/Signal/SignalBase.hpp"

namespace Ego {

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


} // namespace Ego
