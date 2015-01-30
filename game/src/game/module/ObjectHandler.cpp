#include "game/module/ObjectHandler.hpp"
#include "game/char.h"
#include "game/profiles/ProfileSystem.hpp"

const static std::shared_ptr<chr_t> NULL_OBJ = nullptr;

ObjectHandler::ObjectHandler() :
	_characterMap(),
    _characterList(),
	_terminationList(),
    _allocateList(),
    _totalCharactersSpawned(0)
{
	//ctor
}

bool ObjectHandler::remove(const CHR_REF ichr)
{
    if (!exists(ichr))
	{
    	return false;
    }

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && defined(_DEBUG)
    chr_log_script_time( ichr );
#endif

    // If we are inside a list loop, do not actually change the length of the
    // list. Else this can cause some problems later.
    _terminationList.push_back(ichr);

    // At least mark the object as "waiting to be terminated".
    get(ichr)->terminateRequested = true;

    // We can safely modify the map, it is not iterable from the outside.
    _characterMap.erase(ichr);
    
    return true;
}

bool ObjectHandler::exists(const CHR_REF character) const
{
    if(character == INVALID_CHR_REF)
	{
        return false;
    }

    const auto &result = _characterMap.find(character);

	if(result == _characterMap.end())
	{
		return false;
	}

	return !result->second->terminateRequested;
}


std::shared_ptr<chr_t> ObjectHandler::insert(const PRO_REF profile, const CHR_REF override)
{
	// Make sure the profile is valid.
    if(!_profileSystem.isValidProfileID(profile))
    {
        log_warning("ObjectHandler - Tried to spawn character with invalid ProfileID: %d\n", profile);
        return nullptr;
    }

    // Limit total number of characters active at the same time.
    if(getObjectCount() > MAX_CHR)
	{
        log_warning("ObjectHandler - No free character slots available\n");
        return nullptr;
    }

    CHR_REF ichr = INVALID_CHR_REF;

    if(override != INVALID_CHR_REF)
    {
        if(_characterMap.find(override) == _characterMap.end())
        {
            ichr = override;
        }
        else
        {
            log_warning( "ObjectHandler - failed to override a character? character %d already spawned? \n", REF_TO_INT( override ) );
			return nullptr;
		}
    }
    else
    {
        // Increment counter.
        ichr = _totalCharactersSpawned++;
    }

    if (ichr != INVALID_CHR_REF)
    {
        const std::shared_ptr<chr_t> object = std::make_shared<chr_t>(profile, ichr);

        if(!object)
		{
            log_warning("ObjectHandler - Unable to allocate object memory\n");
            return nullptr;
        }

        // Allocate the new one (we can safely modify the map, it isn't iterable from outside).
        if(_characterMap.emplace(ichr, object).second == false)
		{
            log_warning("ObjectHandler - Failed character allocation, object already exists\n");
            return nullptr;
        }

        // Wait to adding it to the iterable list.
        _allocateList.push_back(object);
        return object;
    }

    return nullptr;
}

chr_t* ObjectHandler::get(const CHR_REF index) const
{
    if(index == INVALID_CHR_REF)
	{
        return nullptr;
    }

    const auto &result = _characterMap.find(index);

    if(result == _characterMap.end())
    {
        return nullptr;
    }

    return (*result).second.get();
}

const std::shared_ptr<chr_t>& ObjectHandler::operator[] (const CHR_REF index)
{
    if(index == INVALID_CHR_REF)
	{
        return NULL_OBJ;
    }

    const auto &result = _characterMap.find(index);

    if(result == _characterMap.end())
    {
        return NULL_OBJ;
    }

    return (*result).second;
}

void ObjectHandler::clear()
{
	_characterMap.clear();
	_characterList.clear();
    _terminationList.clear();
    _totalCharactersSpawned = 0;
}

void ObjectHandler::lock()
{
    _semaphore++;
}

#if defined(_DEBUG)
#include <iostream>
#endif

#if defined(_DEBUG)
void ObjectHandler::dumpAllocateList()
{
	if (_allocateList.empty())
	{
		std::cout << "Allocate List = []" << std::endl;
	}
	else
	{
		std::cout << "Allocate List" << std::endl;
		std::cout << "{" << std::endl;
		for (const auto chr : _allocateList)
		{
			if (chr == nullptr)
			{
				std::cout << "  " << "null" << std::endl;
			}
			else
			{
				std::cout << "  " << chr->getCharacterID() << std::endl;
			}
		}
		std::cout << "}" << std::endl;
	}
}
#endif

#if defined(_DEBUG)
void ObjectHandler::dumpTerminationList()
{
	if (_terminationList.empty())
	{
		std::cout << "Termination List = []" << std::endl;
	}
	else
	{
		std::cout << "Termination List" << std::endl;
		std::cout << "{" << std::endl;
		for (const CHR_REF ichr : _terminationList)
		{
			std::cout << "  " << ichr << std::endl;
		}
		std::cout << "}" << std::endl;
	}
}
#endif

void ObjectHandler::unlock()
{
    if(_semaphore == 0) 
    {
        throw new std::logic_error("ObjectHandler calling unlock() without lock()");
    }

    // Release one lock.
    _semaphore--;

    // No remaining locks?
    if(_semaphore == 0)
    {
        // Add any allocated objects to the containers (do first, in case they get removed again).
        for(const std::shared_ptr<chr_t> &object : _allocateList)
        {
            _characterList.push_back(object);
        }
        _allocateList.clear();

        // Go through and delete any characters that were
        // supposed to be deleted while the list was iterating.
        for(const CHR_REF ichr : _terminationList)
        {
            _characterList.erase
			(
                std::remove_if
				(
					_characterList.begin(), _characterList.end(),
					[ichr](const std::shared_ptr<chr_t> &element) 
					{
						return element->terminateRequested || element->getCharacterID() == ichr;
					}
				),
				_characterList.end()
			);
        }
        _terminationList.clear();
    }
}

ObjectHandler::ObjectIterator ObjectHandler::iterator()
{
    return ObjectIterator(this);
}
