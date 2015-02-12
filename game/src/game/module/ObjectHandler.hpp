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
/// @details Memory and access managment for instances of in-game egoboo objects (GameObject)
/// @author Johan Jansen

#pragma once

#include "game/egoboo_typedef.h"

//Forward declarations
class GameObject;

//ZF> Some macros from C Egoboo (TODO: remove these macros)
#define VALID_CHR_RANGE( ICHR )    ( static_cast<CHR_REF>(ICHR) < MAX_CHR && ICHR != INVALID_CHR_REF )
#define GET_INDEX_PCHR( PCHR )     ((nullptr == (PCHR)) ? INVALID_CHR_REF : PCHR->getCharacterID())
#define ACTIVE_PCHR( PCHR )        ( nullptr != (PCHR) && !PCHR->isTerminated() )
#define TERMINATED_PCHR( PCHR )    ( nullptr != (PCHR) && PCHR->isTerminated() )
#define INGAME_PCHR(PCHR)          ( nullptr != (PCHR) && !PCHR->isTerminated() )

/**
* @brief A completely recursive loop safe container for accessing instances of in-game objects
**/
class ObjectHandler
{
public:

	class ObjectIterator
	{
	public:

		inline std::vector<std::shared_ptr<GameObject>>::const_iterator cbegin() const 
		{
			return _handler->_iteratorList.cbegin();
		}

		inline std::vector<std::shared_ptr<GameObject>>::const_iterator cend() const 
		{
			return _handler->_iteratorList.cend();
		}

		inline std::vector<std::shared_ptr<GameObject>>::iterator begin()
		{
			return _handler->_iteratorList.begin();
		}

		inline std::vector<std::shared_ptr<GameObject>>::iterator end()
		{
			return _handler->_iteratorList.end();
		}	

		~ObjectIterator()
		{
			//Free the ObjectHandler lock
			_handler->unlock();
		}

	private:
		ObjectIterator(ObjectHandler *handler) :
			_handler(handler)
		{
			// Ensure the ObjectHandler is locked as long as we are in existance.
			_handler->lock();
		}

		ObjectHandler *_handler;

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
	 * @brief Remove the specified CHR_REF for the game. 
	 * @return false if it didnt exist or was already removed, true otherwise
	 */
	bool remove(const CHR_REF ichr);

	/**
	 * @brief Get if the specified CHR_REF exists and is not terminated yet.
	 * @return true if the specified CHR_REF exists and is not terminated yet
	 */
	bool exists(const CHR_REF ichr) const;

	/**
	 * @brief Allocates and creates new GameObject object. A valid PRO_REF is required to spawn a object.
	 * @return the std::shared_ptr<GameObject> for that object or nullptr if it failed
	 */
	std::shared_ptr<GameObject> insert(const PRO_REF profile, const CHR_REF override = INVALID_CHR_REF);

	/**
	 * @brief Return a pointer object for the specifiec CHR_REF.
	 * @return a pointer object for the specified CHR_REF.
	 *		   Return nullptr object if CHR_REF was not found.
	 */
	const std::shared_ptr<GameObject>& operator[] (const CHR_REF index);

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
	 * @brief Return a raw pointer referenced by CHR_REF
	 * @return a raw pointer referenced by CHR_REF
	 */
	GameObject* get(const CHR_REF index) const;

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
    std::vector<std::shared_ptr<GameObject>> _internalCharacterList;        ///< Indexes in this character list match CHR_REF
	std::vector<std::shared_ptr<GameObject>> _iteratorList;					///< For iterating, contains only valid objects (unsorted)
	std::stack<CHR_REF> _unusedChrRefs;						                ///< Stack of unused CHR_REF

	std::vector<std::shared_ptr<GameObject>> _allocateList;					///< List of all objects that should be added

	size_t _semaphore;
	size_t _deletedCharacters;

	CHR_REF _totalCharactersSpawned;										///< Total count of characters spawned (includes removed)

	friend class ObjectIterator;
};