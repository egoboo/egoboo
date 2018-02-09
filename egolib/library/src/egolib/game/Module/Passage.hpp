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

/// @file egolib/game/Module/Passage.hpp
/// @details Passages and doors and whatnot.  Things that impede your progress!
/// @author Johan Jansen

#pragma once

#include "egolib/game/egoboo.h"
#include "egolib/Mesh/Info.hpp"

//Forward declarations
class Object;
class GameModule;

class Passage
{
public:
    static constexpr int32_t NO_MUSIC = -1;		    ///< For passages that play no music
    static constexpr uint32_t CLOSE_TOLERANCE = 3;  ///< For closing doors
    static constexpr size_t MAX_PASSAGES = 256;	    ///< Maximum allowed passages
    static const ObjectRef SHOP_NOOWNER;	        ///< Shop has no owner
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
	* @brief Constructor
	**/
	Passage(GameModule& module, const int x0, const int y0, const int x1, const int y1, const uint8_t mask);

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
	bool objectIsInPassage(const std::shared_ptr<Object> &object) const;

	/**
	* @brief This function makes a passage flash the specified color
	**/
	void flashColor(uint8_t color);

    /**
    * @brief This function returns ObjectRef::Invalid if there is no object in the passage,
    *    	 otherwise the index of the first object found is returned.
	* @remark Can also look for objects with a specific quest or item in his or her inventory
    *    	  First finds living ones, then items and corpses
    * @return the object reference of the object found which fullfills all specified requirements or ObjectRef::Invalid if none found
    **/
    ObjectRef whoIsBlockingPassage(ObjectRef objRef, const IDSZ2& idsz, const BIT_FIELD targeting_bits, const IDSZ2& require_item) const;


    /**
    * @brief This return true if the specified X and Y coordinates are within the passage
    **/
    bool isPointInside(float x, float y) const;

    /**
    * @brief Plays the passage music assigned to this passage if the specified character is inside this passage
    * @return true if a new song is now playing
    **/
    bool checkPassageMusic(const std::shared_ptr<Object> &pobj) const;

    /**
    * @return Sets the MusicID of this passage. If a player character enters this passage, the specified MusicID will be played
    **/
    void setMusic(const int32_t musicID);

    /**
    * @return true if this passage is a shop
    **/
    bool isShop() const;

	ObjectRef getShopOwner() const;

    void makeShop(ObjectRef owner);

    void removeShop();

    /**
    * @brief
    *	Get the AABB for area that this passage covers.
    **/
    const Ego::AxisAlignedBox2f& getAxisAlignedBox2f() const;

private:
    GameModule& _module;			   ///< Reference to the module we are inside

    Ego::AxisAlignedBox2f _area;	   ///< Passage area
    int32_t _music;   				   ///< Music track appointed to the specific passage
    uint8_t _mask;  				   ///< Is it IMPASSABLE, SLIPPERY or whatever
    bool _open;   					   ///< Is the passage open?

    bool _isShop;					   ///< True if this passage is a shop
    ObjectRef _shopOwner;			   ///< object reference of the owner of this shop
    std::vector<Index1D> _passageFans; //List of all tile indexes contained in this passage
};
