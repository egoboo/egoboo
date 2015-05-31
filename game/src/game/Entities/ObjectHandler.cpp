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

#define GAME_ENTITIES_PRIVATE 1
#include "game/Entities/ObjectHandler.hpp"
#include "game/char.h"
#include "egolib/Profiles/_Include.hpp"

CHR_REF GET_INDEX_PCHR(const Object *pobj)
{
    return (nullptr == pobj) ? INVALID_CHR_REF : pobj->getCharacterID();
}
CHR_REF GET_INDEX_PCHR(const std::shared_ptr<Object> pobj)
{
    return (nullptr == pobj) ? INVALID_CHR_REF : pobj->getCharacterID();
}
bool INGAME_PCHR(const Object *pobj)
{
    return (nullptr != pobj) && !pobj->isTerminated();
}

ObjectHandler::ObjectHandler() :
	_internalCharacterList(),
    _iteratorList(),
    _unusedChrRefs(),
    _allocateList(),

    _semaphore(0),
    _deletedCharacters(0),
    _totalCharactersSpawned(0)
{
	_internalCharacterList.reserve(OBJECTS_MAX);
    _iteratorList.reserve(OBJECTS_MAX);
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
    _internalCharacterList[ichr]->detatchFromHolder(true, false);

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
    if (!ProfileSystem::get().isValidProfileID(profile))
    {
        log_warning("ObjectHandler - Tried to spawn character with invalid ProfileID: %d\n", profile);
        return nullptr;
    }

    // Limit total number of characters active at the same time.
    if(getObjectCount() > OBJECTS_MAX)
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
        return Object::INVALID_OBJECT;
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

    //Lock list while running deferred
    _semaphore = 1;

	// Add any allocated objects to the containers (do first, in case they get removed again).
    if(!_allocateList.empty())
    {
        for (const std::shared_ptr<Object>& object : _allocateList)
        {
            EGOBOO_ASSERT(nullptr != object);
            _iteratorList.push_back(object);
        }
        _allocateList.clear();        
    }

	// Go through and delete any characters that were
	// supposed to be deleted while the list was iterating.
    if(_deletedCharacters > 0) 
    {
        auto condition = 
            [this](const std::shared_ptr<Object>& element)
            {
                EGOBOO_ASSERT(nullptr != element);
                if (element->bsp_leaf.isInList()) return false;

                if (element->isTerminated())
                {
                    //Delete this character
                    _unusedChrRefs.push(element->getCharacterID());
                    _deletedCharacters--;

                    // Make sure everyone knows it died
                    for (const std::shared_ptr<Object>& chr : _iteratorList)
                    {
                        // std::remove_if results in temporary nullptrs in _iteratorList.
                        if (nullptr == chr) continue;
                        //Don't do ourselves or terminated characters
                        if (chr->isTerminated() || chr == element) continue;
                        ai_state_t *ai = chr_get_pai(chr->getCharacterID());

                        if (ai->target == element->getCharacterID())
                        {
                            SET_BIT(ai->alert, ALERTIF_TARGETKILLED);
                        }

                        if (chr_get_pteam(chr->getCharacterID())->getLeader() == element)
                        {
                            SET_BIT(ai->alert, ALERTIF_LEADERKILLED);
                        }
                    }
                    return true;
                }

                return false;
            };
        _iteratorList.erase(std::remove_if(_iteratorList.begin(), _iteratorList.end(), condition),_iteratorList.end());
    }

    //Finally unlock list
    _semaphore = 0;
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
    return ObjectIterator(*this);
}

size_t ObjectHandler::getObjectCount() const 
{
    return _iteratorList.size() + _allocateList.size() - _deletedCharacters;
}

/// @todo Remove this global.
ObjectHandler _gameObjects;