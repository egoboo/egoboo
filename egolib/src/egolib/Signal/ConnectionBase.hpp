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

/// @file egolib/Signal/ConnectionBase.hpp
/// @brief Non-generic base class of all connections.

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Internal {

// Forward declarations.
struct NodeBase;
struct SignalBase;
	
/// Non-generic base class of all connections.
struct ConnectionBase {
    /// A pointer to the node.
    NodeBase *node;

    /// Default construct this connection.
    /// @post This connection is not connected.
    ConnectionBase();

    /// Construct this connection with the specified arguments.
    /// @param node a pointer to a node of the signal
    ConnectionBase(NodeBase *node);

    /// Copy construct this connection with the values of another connection.
    /// @param other the other connection
    ConnectionBase(const ConnectionBase& other);

    /// Destruct this connection.
    virtual ~ConnectionBase();

    const ConnectionBase& operator=(const ConnectionBase& other);

    bool operator==(const ConnectionBase& other) const;

    bool operator!=(const ConnectionBase& other) const;


    /// @brief Get if the connection is connected.
    /// @return @a true if this connection is connected, @a false otherwise
    bool isConnected() const;

    /// @brief Disconnect this connection.
    void disconnect();

private:
    void reset();

}; // struct Connection


	
} // namespace Internal
} // namespace Ego
