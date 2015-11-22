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

ObjectRef GET_INDEX_PCHR(const Object *pobj) {
    return (nullptr == pobj) ? ObjectRef::Invalid : pobj->getObjRef();
}
ObjectRef GET_INDEX_PCHR(const std::shared_ptr<Object> &pobj) {
    return (nullptr == pobj) ? ObjectRef::Invalid : pobj->getObjRef();
}
bool INGAME_PCHR(const Object *pobj)
{
    return (nullptr != pobj) && !pobj->isTerminated();
}

ObjectHandler::ObjectHandler() :
	_internalCharacterList(),
    _iteratorList(),
    _allocateList(),

    _semaphore(0),
    _deletedCharacters(0),
    _totalCharactersSpawned(0),
    _dynamicObjects(),
    _staticObjects(),
    _updateStaticTreeClock(0)
{
    _iteratorList.reserve(OBJECTS_MAX);
}

bool ObjectHandler::remove(ObjectRef ref) {
	if (!exists(ref)) {
		return false;
	}

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && defined(_DEBUG)
	chr_log_script_time(ref.get());
#endif

	//Remove us from any holder first
	_internalCharacterList[ref]->detatchFromHolder(true, false);

	// If we are inside a list loop, do not actually change the length of the
	// list. Else this can cause some problems later.
	_internalCharacterList[ref]->_terminateRequested = true; //bad: private access
	_deletedCharacters++;

	// We can safely modify the map, it is not iterable from the outside.
	_internalCharacterList.erase(ref);

	return true;
}

bool ObjectHandler::exists(ObjectRef ref) const {
	if (ref == ObjectRef::Invalid || ref.get() >= _totalCharactersSpawned) {
		return false;
	}

	// Check if object exists in map.
	const auto& result = _internalCharacterList.find(ref);
	if (result == _internalCharacterList.end()) {
		return false;
	}

	return !(*result).second->isTerminated();
}

std::shared_ptr<Object> ObjectHandler::insert(const PRO_REF profileRef, ObjectRef overrideRef)
{
	// Make sure the profile is valid.
	if (!ProfileSystem::get().isValidProfileID(profileRef)) {
		Log::get().warn("%s:%d: tried to spawn object with invalid profile reference %d\n", __FILE__, __LINE__, profileRef);
		return nullptr;
	}

	// Limit total number of characters active at the same time.
	if (getObjectCount() > OBJECTS_MAX) {
		Log::get().warn("%s:%d: no free object slots available\n", __FILE__, __LINE__);
		return nullptr;
	}

	ObjectRef objRef = ObjectRef::Invalid;

	if (ObjectRef::Invalid != overrideRef) {
		if (!exists(overrideRef)) {
			objRef = overrideRef;
		} else {
			Log::get().warn("%s:%d: failed to override a object %" PRIuZ ": - object already spawned\n", __FILE__, __LINE__,\
				            overrideRef.get());
			return nullptr;
		}
	}
	// No override specified, generate new reference.
	else
	{
		// Increment counter.
		objRef = ObjectRef(_totalCharactersSpawned++);
	}

	if (ObjectRef::Invalid != objRef) {
		const std::shared_ptr<Object> objPtr = std::make_shared<Object>(profileRef, objRef);
		if (!objPtr) {
			Log::get().warn("unable to create object\n");
			return nullptr;
		}

		// Allocate the new one (we can safely modify the internal map, it isn't iterable from outside).
		_internalCharacterList[objRef] = objPtr;

		// Wait to adding it to the iterable list.
		_allocateList.push_back(objPtr);
		return objPtr;
	}

	return nullptr;
}

Object *ObjectHandler::get(ObjectRef ref) const {
	if (ref == ObjectRef::Invalid || ref.get() >= _totalCharactersSpawned) {
		return nullptr;
	}

	// Check if object exists in map.
	const auto& result = _internalCharacterList.find(ref);
	if (result == _internalCharacterList.end()) {
		return nullptr;
	}

	return (*result).second.get();
}

const std::shared_ptr<Object>& ObjectHandler::operator[] (ObjectRef ref)
{
	if (ref == ObjectRef::Invalid || ref.get() >= _totalCharactersSpawned) {
		return Object::INVALID_OBJECT;
	}

	// Check if object exists in map.
	const auto& result = _internalCharacterList.find(ref);
	if (result == _internalCharacterList.end()) {
		return Object::INVALID_OBJECT;
	}

	return (*result).second;
}

void ObjectHandler::clear()
{
	_internalCharacterList.clear();
	_iteratorList.clear();
    _dynamicObjects.clear(0, 0, 0, 0);
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
				std::cout << "  " << chr->getObjRef().get() << std::endl;
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

                if (element->isTerminated())
                {
                    //Delete this character
                    _deletedCharacters--;

                    // Make sure everyone knows it died
                    for (const std::shared_ptr<Object>& chr : _iteratorList)
                    {
                        // std::remove_if results in temporary nullptrs in _iteratorList.
                        if (nullptr == chr) continue;
                        
                        //Don't do ourselves or terminated characters
                        if (chr->isTerminated() || chr == element) continue;
						ai_state_t *ai = &(chr->ai);

                        if (ai->target == element->getObjRef().get())
                        {
                            SET_BIT(ai->alert, ALERTIF_TARGETKILLED);
                        }

                        if (chr->getTeam().getLeader() == element)
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

void ObjectHandler::updateQuadTree(float minX, float minY, float maxX, float maxY)
{
    //Reset quad-tree
    _dynamicObjects.clear(minX, minY, maxX, maxY);

    //Rebuild the static quad tree only once per second
    bool updateStaticQuadTree = false;
    if(_updateStaticTreeClock <= 0) {
        _updateStaticTreeClock = ONESECOND;
        updateStaticQuadTree = true;
        _staticObjects.clear(minX, minY, maxX, maxY);
    }
    else {
        _updateStaticTreeClock--;
    }

    //Rebuild quad-tree
    for(const std::shared_ptr<Object> &object : _iteratorList) {
        //Do not add objects that cannot interact with the rest of the world
        if(object->isTerminated() || object->isHidden()) continue;

        if(object->isScenery()) {
            if(updateStaticQuadTree) {
                _staticObjects.insert(object);
            }
        }
        else {
            _dynamicObjects.insert(object);
        }
    }
}

std::vector<std::shared_ptr<Object>> ObjectHandler::findObjects(const float x, const float y, const float distance, bool includeSceneryObjects) const { 
    std::vector<std::shared_ptr<Object>> result;
	AABB2f searchArea = AABB2f(Vector2f(x-distance, y-distance), Vector2f(x+distance, y+distance));
    _dynamicObjects.find(searchArea, result);
    if(includeSceneryObjects) _staticObjects.find(searchArea, result);
    return result;
}

void ObjectHandler::findObjects(const AABB2f &searchArea, std::vector<std::shared_ptr<Object>> &result, bool includeSceneryObjects) const
{
    if(includeSceneryObjects) _staticObjects.find(searchArea, result);
    return _dynamicObjects.find(searchArea, result);
}
