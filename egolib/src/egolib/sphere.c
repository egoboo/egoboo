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

/// @file egolib/sphere.c
/// @brief spheres
#include "egolib/sphere.h"

sphere_t *sphere_ctor(sphere_t *self)
{
#if 0
	EGOBOO_ASSERT(nullptr != self);
#endif
	if (nullptr == self) return self;
	self->radius = -1.0f;
	fvec3_ctor(self->origin.v);
#if 0
	if (nullptr == self) return self;
#endif
	return self;
}

sphere_t *sphere_dtor(sphere_t *self)
{
	if (nullptr == self) return self;
	fvec3_dtor(self->origin.v);
	self->radius = 0.0f;
	return self;
}


bool sphere_self_clear(sphere_t *self)
{
	if (nullptr == self) return false;
	fvec3_self_clear(self->origin.v);
	self->radius = 0.0f;
	return true;
}

bool sphere_self_is_clear(const sphere_t *self)
{
	if (nullptr == self) return true;
	return 0.0f == self->radius
		&& fvec3_self_is_clear(self->origin.v);
}