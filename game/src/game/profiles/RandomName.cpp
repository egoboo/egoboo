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
/// @file  game/RandomName.cpp
/// @brief This class is used for random name generation
/// @author Johan Jansen

#include "game/profiles/RandomName.hpp"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/math/Random.hpp"

RandomName::RandomName() :
	_randomNameBlocks()
{
	//ctor
}

std::string RandomName::generateRandomName() const
{
	if(_randomNameBlocks.empty()) {
		return "*NONE*";
	}

	std::string result;

	//Build random name by picking a random part from each loaded block
	for(const std::vector<std::string> &block : _randomNameBlocks)
	{
		result += Random::getRandomElement(block);
	}

	return result;
}

bool RandomName::loadFromFile(const std::string &filePath)
{
	vfs_FILE *fileRead = vfs_openRead(filePath.c_str());

	//Unable to open file
	if(!fileRead) {
		return false;
	}

	//Clear any old random name data
	_randomNameBlocks.clear();

	//Parse the entire file
	std::vector<std::string> *currentBlock = nullptr;
	while(goto_colon_vfs(nullptr, fileRead, true))
	{
		char buffer[256];
        vfs_get_string(fileRead, buffer, SDL_arraysize(buffer));

        // convert all the '_' and junk in the string
        str_decode(buffer, SDL_arraysize(buffer), buffer);

        const std::string line = buffer;

        //End of random name block is marked with STOP
        if(line == "STOP") 
        {
        	//Prevent empty name blocks
        	if(currentBlock != nullptr && currentBlock->empty()) {
        		_randomNameBlocks.pop_back();
        	}

        	currentBlock = nullptr;
        }
        else
        {
			//Create next block of random name parts
        	if(currentBlock == nullptr)
        	{
                _randomNameBlocks.push_back(std::vector<std::string>());
                currentBlock = &_randomNameBlocks.back();
        	}

            //Add this name part to this block
            currentBlock->push_back(line);
        }
	}

	//Release file resource
    vfs_close(fileRead);

	return !_randomNameBlocks.empty();
}

bool RandomName::exportName(const std::string &name, const std::string &filePath)
{
    //Make sure exported name is valid
    if(name.empty()) {
        return false;
    }

    //Open file for writing
    vfs_FILE* fileWrite = vfs_openWrite(filePath.c_str());
    if(!fileWrite) {
        return false;
    }

    //Prefix with colon
    vfs_printf( fileWrite, ":" );

    //Replace whitespace with underscore
    for(const char &character : name) {
        if(character == ' ') {
            vfs_printf(fileWrite, "_" );
        }
        else {
            vfs_printf(fileWrite, "%c", character);
        }
    }

    //finished!
    vfs_printf(fileWrite, "%s\n", name.c_str());
    vfs_printf(fileWrite, ":STOP\n\n");
    vfs_close(fileWrite);
    return true;
}
