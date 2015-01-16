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
/// @file  game/RandomName.hpp
/// @brief This class is used for random name generation
/// @author Johan Jansen

#pragma once
#include <vector>
#include <string>

class RandomName
{
public:
	RandomName();

    /**
    * @details This function generates a random name.  Return "*NONE*" on a failure.
    **/
	std::string generateRandomName() const;

	/**
	* @details Loads a naming.txt file
	* @return true if at least 1 name was successfuly loaded
	**/
	bool loadFromFile(const std::string &filePath);

    /**
    * @details This function exports a simple string to the naming.txt file
    * @todo: ZF> this function should probably not be in here and should be moved to char.h
    * @return true if it was successfully exported
    **/
	static bool exportName(const std::string &name, const std::string &filePath);

	inline bool isLoaded() const {return !_randomNameBlocks.empty();}

private:
	std::vector<std::vector<std::string>> _randomNameBlocks;
};
