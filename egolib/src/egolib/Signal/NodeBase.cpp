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
    : next(nullptr), dead(false) {}

NodeBase::~NodeBase() {}

bool NodeBase::kill() noexcept {
    bool temporary = dead;
    dead = true;
    return !temporary;
}

bool NodeBase::isDead() const noexcept {
    return dead;
}

} // namespace Internal
} // namespace Ego
