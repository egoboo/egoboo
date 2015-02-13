#include "game/entities/ObjectHandler.hpp"
#include "game/char.h"
#include "game/profiles/ProfileSystem.hpp"

const static std::shared_ptr<Object> NULL_OBJ = nullptr;

ObjectHandler::ObjectHandler() :
	_internalCharacterList(),
    _iteratorList(),
    _unusedChrRefs(),
    _allocateList(),

    _semaphore(0),
    _deletedCharacters(0),
    _totalCharactersSpawned(0)
{
	_internalCharacterList.reserve(MAX_CHR);
    _iteratorList.reserve(MAX_CHR);
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

    //Remove us from any holder first
    detach_character_from_mount(ichr, true, false);

    // If we are inside a list loop, do not actually change the length of the
    // list. Else this can cause some problems later.
    _internalCharacterList[ichr]->_terminateRequested = true; //bad: private access
    _deletedCharacters++;

    // We can safely modify the map, it is not iterable from the outside.
    _internalCharacterList[ichr] = nullptr;
    
    return true;
}

bool ObjectHandler::exists(const CHR_REF character) const
{
    if(character == INVALID_CHR_REF || character >= _internalCharacterList.size()) 
	{
        return false;
    }

    return _internalCharacterList[character] != nullptr && !_internalCharacterList[character]->isTerminated();
}


std::shared_ptr<Object> ObjectHandler::insert(const PRO_REF profile, const CHR_REF override)
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
        if(!exists(override))
        {
            ichr = override;
        }
        else
        {
            log_warning( "ObjectHandler - failed to override a character? character %d already spawned? \n", REF_TO_INT( override ) );
			return nullptr;
		}
    }

    //No override specified, get first free slot
    else
    {
        if(!_unusedChrRefs.empty())
        {
            ichr = _unusedChrRefs.top();
            _unusedChrRefs.pop();
        }
        else
        {
            ichr = _internalCharacterList.size() + 1;
        }

        // Increment counter.
        _totalCharactersSpawned++;
    }

    if (ichr != INVALID_CHR_REF)
    {
        const std::shared_ptr<Object> object = std::make_shared<Object>(profile, ichr);

        if(!object)
		{
            log_warning("ObjectHandler - Unable to allocate object memory\n");
            return nullptr;
        }

        //Ensure character list is big enough to hold new object
        if(_internalCharacterList.size() < ichr)
        {
            _internalCharacterList.resize(ichr+1);
        }

        // Allocate the new one (we can safely modify the internal list, it isn't iterable from outside).
        _internalCharacterList[ichr] = object;

        // Wait to adding it to the iterable list.
        _allocateList.push_back(object);
        return object;
    }

    return nullptr;
}

Object* ObjectHandler::get(const CHR_REF index) const
{
    if(index == INVALID_CHR_REF || index >= _internalCharacterList.size())
	{
        return nullptr;
    }

    return _internalCharacterList[index].get();
}

const std::shared_ptr<Object>& ObjectHandler::operator[] (const CHR_REF index)
{
    if(index == INVALID_CHR_REF || index >= _internalCharacterList.size())
    {
        return NULL_OBJ;
    }

    return _internalCharacterList[index];
}

void ObjectHandler::clear()
{
    while(!_unusedChrRefs.empty()) _unusedChrRefs.pop();
	_internalCharacterList.clear();
	_iteratorList.clear();
    _deletedCharacters = 0;
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
		for (const auto &chr : _allocateList)
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

void ObjectHandler::maybeRunDeferred()
{
	// If locked ...
	if (_semaphore != 0)
	{
		// ... return.
		return;
	}

	// Add any allocated objects to the containers (do first, in case they get removed again).
    if(!_allocateList.empty())
    {
        for (const std::shared_ptr<Object> &object : _allocateList)
        {
            assert(object != nullptr);
            _iteratorList.push_back(object);
        }
        _allocateList.clear();        
    }

	// Go through and delete any characters that were
	// supposed to be deleted while the list was iterating.
    if(_deletedCharacters > 0) 
    {
        _iteratorList.erase(
            std::remove_if(_iteratorList.begin(), _iteratorList.end(),
                [this](const std::shared_ptr<Object> &element)
                {
                    if (element->bsp_leaf.isInList()) return false;

                    if(element->isTerminated())
                    {
                        //Delete this character
                        _unusedChrRefs.push(element->getCharacterID());
                        _deletedCharacters--;
                        return true;
                    }

                    return false;
                }), 
            _iteratorList.end()
        );
    }
}


void ObjectHandler::unlock()
{
    if(_semaphore == 0) 
    {
        throw new std::logic_error("ObjectHandler calling unlock() without lock()");
    }

    // Release one lock.
    _semaphore--;

    // Run deferred updates if the object list is not locked.
	maybeRunDeferred();
}

ObjectHandler::ObjectIterator ObjectHandler::iterator()
{
    return ObjectIterator(this);
}

size_t ObjectHandler::getObjectCount() const 
{
    return _iteratorList.size() + _allocateList.size() - _deletedCharacters;
}
