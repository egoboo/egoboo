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

/// @file game/Entities/ObjectHandler.hpp
/// @details Memory and access managment for instances of in-game egoboo objects (Object)
/// @author Johan Jansen

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif

#include "game/egoboo.h"
#include "egolib/Core/QuadTree.hpp"

//Forward declarations
class Object;

//ZF> Some macros from C Egoboo (TODO: remove these macros)
ObjectRef GET_INDEX_PCHR(const Object *pobj);
ObjectRef GET_INDEX_PCHR(const std::shared_ptr<Object> pobj);
bool INGAME_PCHR(const Object *pobj);

/**
* @brief A completely recursive loop safe container for accessing instances of in-game objects
**/
class ObjectHandler : private id::non_copyable
{
public:

	class ObjectIterator
	{
	public:

		inline std::vector<std::shared_ptr<Object>>::const_iterator cbegin() const 
		{
			return _handler._iteratorList.cbegin();
		}

		inline std::vector<std::shared_ptr<Object>>::const_iterator cend() const 
		{
			return _handler._iteratorList.cend();
		}

		inline std::vector<std::shared_ptr<Object>>::iterator begin()
		{
			return _handler._iteratorList.begin();
		}

		inline std::vector<std::shared_ptr<Object>>::iterator end()
		{
			return _handler._iteratorList.end();
		}	

		~ObjectIterator()
		{
			//Free the ObjectHandler lock
			_handler.unlock();
		}

        // Copy constructor
        ObjectIterator(const ObjectIterator &other) :
            _handler(other._handler)
        {
            _handler.lock();
        }
        
	    // Disable copy assignment operator
	    ObjectIterator& operator=(const ObjectIterator&) = delete;
    
	private:
		ObjectIterator(ObjectHandler &handler) :
			_handler(handler)
		{
			// Ensure the ObjectHandler is locked as long as we are in existance.
			_handler.lock();
		}

		ObjectHandler &_handler;

		friend class ObjectHandler;
	};

public:
	/**
	 * @brief Default constructor.
	 */
	ObjectHandler();

	/**
	 * @brief Returns a safe deferred iterator that ensure nothing is modified while iterating.
	 */
	ObjectIterator iterator();

	/**
	 * @brief Remove the specified object for the game. 
	 * @return false if it didnt exist or was already removed, true otherwise
	 */
	bool remove(ObjectRef ref);

	/**
	 * @brief Get if the specified object exists and is not terminated yet.
	 * @return true if the specified object exists and is not terminated yet
	 */
	bool exists(ObjectRef ref) const;

	/**
	 * @brief Allocates and creates new Object object. A valid object profile reference is required to spawn a object.
	 * @return the std::shared_ptr<Object> for that object or nullptr if it failed
	 */
	std::shared_ptr<Object> insert(ObjectProfileRef profileRef, ObjectRef overrideRef = ObjectRef::Invalid);

	/**
	 * @brief Return a pointer object for the specifiec object reference.
	 * @return a pointer object for the specified object reference.
	 *		   Return nullptr object if the object reference was not found.
	 */
	const std::shared_ptr<Object>& operator[] (ObjectRef ref);

	/**
	 * @brief Return number of object currently active in the game.
	 * @return number of objects currently active in the game
	 */
	size_t getObjectCount() const;

	/**
	 * @brief Removes and de-allocates all game objects contained in this ObjectHandler.
	 */
	void clear();

	/**
	 * @brief Return a raw pointer to the object referenced by the object reference
	 * @return a raw pointer referenced by the object reference
	 */
	Object *get(ObjectRef ref) const;


	/**
	* @brief
	*	Find all elements that are within range of a specified point in this QuadTree's
	*	bounding box
	* @param x
	*	x position of point to search from
	* @param y
	*	y position of point to search from
	* @param distance
	*	range of search from point
	* @param includeSceneryObjects
	*	if true, it will also include Scenery objects in the search as defined by Object::isScenery()
	* @return
	*	A vector containing all elements that fit the search
	**/
	std::vector<std::shared_ptr<Object>> findObjects(const float x, const float y, const float distance, bool includeSceneryObjects = true) const;

	/**
	* @brief
	*	Find all elements that collide with a 2D bounding box area
	* @param searchArea
	*	The bounding box to scan
	* @param result
	*	reference to the vector where the result is stored
	* @param includeSceneryObjects
	*	if true, it will also include Scenery objects in the search as defined by Object::isScenery()
	**/
	void findObjects(const AxisAlignedBox2f &searchArea, std::vector<std::shared_ptr<Object>> &result, bool includeSceneryObjects = true) const;

	/**
	* @brief
	* 	Clear and rebuild the quad tree for this update frame
	*	This function is NOT thread-safe
	* @param minX, minY, maxX, maxY
	*	Sets the bounds of this quad tree (size of the entire current level)
	**/
	void updateQuadTree(float minX, float minY, float maxX, float maxY);

	/**
	* @return
	*	All objects contained in this ObjectHandler
	**/
	const std::vector<std::shared_ptr<Object>>& getAllObjects() const {return _iteratorList; }

private:

	/**
	 * @brief Locks all object containers to ensure no modification will happen.
	 *		  Must be called before iterating.
	 */
	void lock();

	/**
	 * @brief Unlocks all object containers for modifications.
	 */
	void unlock();

	/**
	 * @brief
	 *	Run all deferred updates if the object list is not locked.
	 */
	void maybeRunDeferred();

#if defined(_DEBUG)
	/**
	 * @brief
	 *	Dump the allocate list to standard output.
	 */
	void dumpAllocateList();
#endif

private:
	Ego::QuadTree<Object> _dynamicObjects;			//Objects that can move (Creatures, moving platforms, etc.)
	Ego::QuadTree<Object> _staticObjects;			//Objects that rarely move - if ever (Trees, pillars, chairs)
	int _updateStaticTreeClock;

	std::unordered_map<ObjectRef, std::shared_ptr<Object>> _internalCharacterList; ///< Maps object references to shared pointers to objects
	std::vector<std::shared_ptr<Object>> _iteratorList;					///< For iterating, contains only valid objects (unsorted)

	std::vector<std::shared_ptr<Object>> _allocateList;					///< List of all objects that should be added

	size_t _semaphore;
	size_t _deletedCharacters;

	size_t _totalCharactersSpawned;										///< Total count of characters spawned (includes removed)

	friend class ObjectIterator;
};
