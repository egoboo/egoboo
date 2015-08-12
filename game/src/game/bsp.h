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

/// @file  game/bsp.h
/// @brief Global mesh, character and particle BSPs.
#pragma once

#include "egolib/typedef.h"

//Forward declarations
namespace Ego {class Particle;}
class Object;

bool prt_BSP_can_collide(const std::shared_ptr<Ego::Particle> &pprt);
bool chr_BSP_can_collide(const std::shared_ptr<Object> &pobj);
