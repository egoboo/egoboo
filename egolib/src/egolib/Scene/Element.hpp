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

#include "egolib/bv.h"

namespace Scene {

/**
 * @brief
 *	The interface and element of a scene must implement.
 */
class Element {

protected:
	
	/**
	 * Construct this scene element.
	 */
	Element() {
	}
	
	/**
	 * Destruct this scene element.
	 */
	virtual ~Element() {
	}

public:

	/**
	 * @brief
	 *	Get the bounding volume of this scene element.
	 * @return
	 *	the bounding volume of this scene element
	 */
	virtual const bv_t& getBoundingVolume() const = 0;

};

};