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

#pragma once

#include <memory>
#include "egolib/Entities/Forward.hpp"
#include "egolib/typedef.h"

bool do_chr_prt_collision(const std::shared_ptr<Object> &object, const std::shared_ptr<Ego::Particle> &particle, const float tmin, const float tmax);
bool do_prt_platform_detection( const ObjectRef ichr_a, const ParticleRef iprt_b );

bool get_prt_mass(Ego::Particle *pprt, Object *pchr, float *wt);
void get_recoil_factors(float wta, float wtb, float * recoil_a, float * recoil_b);
