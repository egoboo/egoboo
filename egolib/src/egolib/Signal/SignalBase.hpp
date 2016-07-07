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

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Internal {

// Forward declarations.
struct ConnectionBase;
struct NodeBase;
struct SignalBase;

/// Non-generic base class of any signal.
struct SignalBase {
    friend struct ConnectionBase;
protected:
    NodeBase *head;   ///< The head of the nodes.
    bool running;     ///< @a true if the signal is currently running, @a false otherwise.
    size_t deadCount; ///< The number of dead nodes.
    size_t liveCount; ///< The number of live nodes.

    /// Remove all dead subscriptions if the number of dead nodes exceeds the number of live nodes.
    /// @precondition The signal is not currently running.
    void maybeSweep() noexcept;

    /// Does the subscriber list need sweeping?
    /// @return @a true if the subscriber list needs sweeping, @a false otherwise
    bool needsSweep() const noexcept;

public:
    SignalBase(const SignalBase&) = delete; // Do not allow copying.
    const SignalBase& operator=(const SignalBase&) = delete; // Do not allow copying.
    
public:
    /// Default constructor.
    SignalBase() noexcept;

    /// Virtual destructor.
    /// Disconnects all subscribers.
    virtual ~SignalBase() noexcept;

    /// Disconnect from this signal.
    /// @param connection the connection
    /// @remark If the connection is not connected or not a connection to this signal, then this function returns immediatly
    void unsubscribe(const ConnectionBase& connection) noexcept;

};

} // namespace Internal
} // namespace Ego
