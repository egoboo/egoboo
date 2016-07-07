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
    /// A pointer to the successor of this node if it has a successor, a null pointer otherwise.
    NodeBase *next;
    /// Is this node dead?
    bool dead;

    NodeBase(const NodeBase&) = delete; // Do not allow copying.
    const NodeBase& operator=(const NodeBase&) = delete; // Do not allow copying.

    /// @brief Default constructor.
    NodeBase();

    /// @brief Virtual destructor.
    virtual ~NodeBase();

    /// @brief Kill this node.
    /// @postcondition this node is dead
    /// @return @a true if the node was killed by this call, @a false otherwise
    bool kill() noexcept;

    /// @brief Get if this node is dead.
    /// @return @a true if this node is dead, @a false otherwise
    bool isDead() const noexcept;

};

} // namespace Internal
} // namespace Ego
