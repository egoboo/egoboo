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

/// @file game/module/ObjectHandler.hpp
/// @details Memory and access managment for instances of in-game egoboo objects (chr_t)
/// @author Johan Jansen

#pragma once

#include <unordered_map>
#include <vector>
#include "game/egoboo_typedef.h"

//Forward declarations
class chr_t;

class ObjectHandler
{
public:
	/**
	* @brief default constructor
	**/
	ObjectHandler();

	/**
	* @brief removes the specified CHR_REF for the game. 
	* @return false if it didnt exist or was already removed, true otherwise
	**/
	bool remove(const CHR_REF ichr);

	/**
	* @return true if the specified CHR_REF exists and is not terminated yet
	**/
	bool exists(const CHR_REF ichr) const;

	/**
	* @brief Allocates and creates new chr_t object. A valid PRO_REF is required to spawn a object
	* @return the CHR_REF for that object or INVALID_CHR_REF if it failed
	**/
	CHR_REF insert(const PRO_REF profile, const CHR_REF override = INVALID_CHR_REF);

	/**
	* @return a raw pointer referenced by CHR_REF
	**/
	chr_t* operator[] (const CHR_REF index);

	/**
	* @return A pointer object for the specified CHR_REF
	*		  Return a nullptr object if CHR_REF was not found
	**/
	const std::shared_ptr<chr_t>&  get(const CHR_REF index) const;

	/**
	* @return Number of objects currently active in the game
	**/
	size_t getObjectCount() const {return _characterList.size();}


	//void forEach(std::function<void>() predicate);

private:
	/**
	* @brief removes all objects marked for removal
	**/
	void cleanup();

private:
	std::vector<CHR_REF> _terminationList;		///< List of all objects that should be terminated
	std::unordered_map<CHR_REF, std::shared_ptr<chr_t>> _characterList;
	int _loopDepth;
};