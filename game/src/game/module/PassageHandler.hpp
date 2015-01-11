#pragma once

#include "game/module/Passage.hpp"

//TODO: remove this file
//The logic in this file should be moved to somewhere else like Module.cpp
//or refactored so that these functions are no longer needed

namespace Passages
{
	//Load all passages from file
	void loadAllPassages();

	//clear passage memory
	void clearPassages();

    /// @author ZF
    /// @details This function checks all passages if there is a player in it, if it is, it plays a specified
    /// song set in by the AI script functions
	void checkPassageMusic();

    /// @author ZZ
    /// @details This function returns the owner of a item in a shop
	CHR_REF getShopOwner(const float x, const float y);

	/**
	* @brief Mark all shop passages having this owner as no longer a shop
	**/
	void removeShopOwner(CHR_REF owner);

	/**
	* @return number of passages currently loaded
	**/
	int getPassageCount();

	/**
	* @brief Get Passage by index number
	* @return nullptr if the id is invalid else the Passage located in the ordered index number
	**/
	std::shared_ptr<Passage> getPassageByID(int id);
}
