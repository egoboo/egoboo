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

/// @file game/Module/Passage.hpp
/// @details Passages and doors and whatnot.  Things that impede your progress!
/// @author Johan Jansen

#pragma once

#include "egolib/typedef.h"
#include "game/egoboo_typedef.h"

//Forward declarations
class Object;

class Passage
{
public:
    static constexpr int32_t NO_MUSIC = -1;				///< For passages that play no music
    static constexpr uint32_t CLOSE_TOLERANCE = 3;		///< For closing doors
    static constexpr size_t MAX_PASSAGES = 256;			///< Maximum allowed passages
    static constexpr CHR_REF SHOP_NOOWNER = 0xFFFF;		///< Shop has no owner
    static constexpr uint32_t SHOP_STOLEN = 0xFFFF;

	/// The pre-defined orders for communicating with shopkeepers
	enum ShopOrders : uint8_t
	{
	    SHOP_BUY,
	    SHOP_SELL,
	    SHOP_NOAFFORD,
	    SHOP_THEFT,
	    SHOP_ENTER,
	    SHOP_EXIT,

	    SHOP_LAST
	};

	/**
	* @brief Default constructor
	**/
	Passage();

	/**
	* @brief Constructor
	**/
	Passage(const irect_t& area, const uint8_t mask);

	/**
	* @brief get left coordinate of passage
	**/
	inline int getLeft() const {return _area._left;}

	/**
	* @brief get top coordinate of passage
	**/
	inline int getTop() const {return _area._top;}

	/**
	* @brief get right coordinate of passage
	**/
	inline int getRight() const {return _area._right;}

	/**
	* @brief get bottom coordinate of passage
	**/
	inline int getBottom() const {return _area._bottom;}

	/**
	* @brief returns true if this passage is currently open (not impassable)
	**/
	bool isOpen() const;

	/**
	* @brief opens this passage
	**/
	void open();

	/**
	* @brief closes this passage
	* @return false if it failed to close (incrushable character inside for example)
	**/
	bool close();

    /**
    * @brief This return true if the specified X and Y coordinates are within the passage
    *    	 radius is how much offset we allow outside the passage
    * @return true if object is inside this passage
    */

	bool objectIsInPassage(float xpos, float ypos, float radius) const;

	/**
	* @brief This function makes a passage flash the specified color
	**/
	void flashColor(uint8_t color);

    /**
    * @brief This function returns INVALID_CHR_REF if there is no character in the passage,
    *    	 otherwise the index of the first character found is returned...
    *    	 Can also look for characters with a specific quest or item in his or her inventory
    *    	 Finds living ones, then items and corpses
    * @return CHR_REF of the character found which fullfills all specified requirements or INVALID_CHR_REF if none found
    **/
    CHR_REF whoIsBlockingPassage( const CHR_REF isrc, IDSZ idsz, const BIT_FIELD targeting_bits, IDSZ require_item ) const;


    /**
    * @brief This return true if the specified X and Y coordinates are within the passage
    **/
    bool isPointInside(float x, float y) const;

    /**
    * @brief Plays the passage music assigned to this passage if the specified character is inside this passage
    * @return true if a new song is now playing
    **/
    bool checkPassageMusic(const Object * pchr) const;

    /**
    * @return Sets the MusicID of this passage. If a player character enters this passage, the specified MusicID will be played
    **/
    void setMusic(const int32_t musicID);

    /**
    * @return true if this passage is a shop
    **/
    bool isShop() const;

    CHR_REF getShopOwner() const;

    void makeShop(CHR_REF owner);

    void removeShop();

private:
    irect_t _area;			///< Passage area
    int32_t _music;   		///< Music track appointed to the specific passage
    uint8_t _mask;  		///< Is it IMPASSABLE, SLIPPERY or whatever
    bool _open;   			///< Is the passage open?

    bool _isShop;			///< True if this passage is a shop
    CHR_REF _shopOwner;		///< CHR_REF of the owner of this shop
};
