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

#include "game/egoboo_typedef.h"

//Forward declarations
class chr_t;

//ZF> Some macros from C Egoboo (TODO: remove these macros)
#define VALID_CHR_RANGE( ICHR )    ( static_cast<CHR_REF>(ICHR) < MAX_CHR && ICHR != INVALID_CHR_REF )
#define GET_INDEX_PCHR( PCHR )      ((nullptr == (PCHR)) ? INVALID_CHR_IDX : PCHR->getCharacterID())
#define ACTIVE_PCHR( PCHR )         ( nullptr != (PCHR) && !PCHR->terminateRequested )
#define TERMINATED_PCHR( PCHR )     ( nullptr != (PCHR) && PCHR->terminateRequested )
#define INGAME_PCHR(PCHR)           ( nullptr != (PCHR) && !PCHR->terminateRequested )

/**
* @brief A completely thread safe container for accessing instances of in-game objects
**/
class ObjectHandler
{
public:

	class ObjectIterator
	{
	public:

		inline std::list<std::shared_ptr<chr_t>>::const_iterator cbegin() const 
		{
			return _handler->_characterList.cbegin();
		}

		inline std::list<std::shared_ptr<chr_t>>::const_iterator cend() const 
		{
			return _handler->_characterList.cend();
		}

		inline std::list<std::shared_ptr<chr_t>>::const_iterator begin()
		{
			return _handler->_characterList.begin();
		}

		inline std::list<std::shared_ptr<chr_t>>::const_iterator end()
		{
			return _handler->_characterList.end();
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
			//Ensure the ObjectHandler is locked as long as we are in existance
			_handler->lock();
		}

		ObjectHandler *_handler;

		friend class ObjectHandler;
	};

public:
	/**
	* @brief default constructor
	**/
	ObjectHandler();

	/**
	* @brief Returns a safe deferred iterator that ensure nothing is modified while iterating
	**/
	ObjectIterator iterator();

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
	* @return A pointer object for the specified CHR_REF
	*		  Return a nullptr object if CHR_REF was not found
	**/
	const std::shared_ptr<chr_t>& operator[] (const CHR_REF index);

	/**
	* @return a raw pointer referenced by CHR_REF
	**/
	chr_t* get(const CHR_REF index) const;

	/**
	* @return Number of objects currently active in the game
	**/
	size_t getObjectCount() const {return _characterList.size();}

	/**
	* @brief Locks all object containers to ensure no modification will happen.
	*		 Must be called before iterating.
	**/
	void lock();

	/**
	* @brief unlocks all object containers for modifications
	**/
	void unlock();

	/**
	* @brief Removes and de-allocates all game objects contained in this ObjectHandler
	**/
	void clear();

private:
	std::unordered_map<CHR_REF, std::shared_ptr<chr_t>> _characterMap;		///< Maps CHR_REF to a chr_t pointer
	std::list<std::shared_ptr<chr_t>> _characterList;						///< For iterating

	std::vector<CHR_REF> _terminationList;									///< List of all objects that should be terminated

	mutable std::recursive_mutex _modificationLock;
	std::atomic_size_t _semaphore;

	CHR_REF _totalCharactersSpawned;										///< Total count of characters spawned (includes removed)

	friend class ObjectIterator;
};