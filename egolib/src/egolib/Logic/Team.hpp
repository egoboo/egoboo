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
/**
 * @brief
 *  Game logic handling of character teams
 * @author
 *  Johan Jansen
 */
#pragma once

#include "egolib/platform.h"
#include "game/egoboo.h"

/// The description of a single team
class Team : public Id::EqualToExpr<Team>
{
public:
    enum TeamTypes : uint8_t
    {
        TEAM_EVIL            = ( 'E' - 'A' ),        ///< Evil team
        TEAM_GOOD            = ( 'G' - 'A' ),        ///< Good team
        TEAM_NULL            = ( 'N' - 'A' ),        ///< Null or Neutral team
        TEAM_ZIPPY           = ( 'Z' - 'A' ),        ///< Zippy Team?
        TEAM_DAMAGE,                                 ///< For damage tiles
        TEAM_MAX
    };

    Team(const TEAM_REF teamID);

    /**
    * @brief
    *   Get the Object whos is the Leader of this team. If no Leader is specifically
    *   set then the first Object joining this team is assigned as the leader.
    *   If there is no leader or the leader has died, then this function will return
    *   a nullptr.
    **/
	std::shared_ptr<Object> getLeader() const;

    /**
    * @brief
    *   Change the leader of this team
    * @param object
    *   The Object who should become the leader. Use Object::INVALID_OBJECT for no 
    *   leader.
    **/
	void setLeader(const std::shared_ptr<Object> &object);

    /**
    * @brief
    *   Issues a call for help for the specified character. All friendly characters
    *   in the game will be notified that this character has called for help.
    * @param caller
    *   The character who is calling for help
    **/
    void callForHelp(const std::shared_ptr<Object> &caller);

    /**
    * @brief
    *   Get the pansy who last has called for help. Returns a nullptr if no one has
    *   called or the caller has died.
    **/
    std::shared_ptr<Object> getSissy() const;

    /**
    * @brief
    *   Check if this teams hates the other team (and can attack them without incurring 
    *   friendly fire). Note that one team can hate the other without the other hating it
    *   back. Nobody hates the neutral (Null) team and the netural team hates nobody.
    * @param other
    *   The other Team to check if this one hates
    * @return
    *   true if this team hates the specified team
    **/
    bool hatesTeam(const Team &other) const;

    /**
    * @brief
    *   Makes this team friendly to another team (but not vice-versa)
    * @param other
    *   the team to make friends with
    **/
    void makeAlliance(const Team &other);

    /**
    * @brief 
    *   This function gives every character on a team experience
    * @param amount
    *   The amount of experience points to award
    * @param xptype
    *   What kind of experience points to award
    **/
    void giveTeamExperience(const int amount, const XPType xptype) const;

    /**
    * @brief
    *   Get number of living characters currently on this team
    **/
    uint16_t getMorale() const;

    /**
    * @brief
    *   Increases team morale by 1
    **/
    void increaseMorale();

    /**
    * @brief
    *   Decreases team morale by 1. Does nothing if morale is already at zero.
    **/
    void decreaseMorale();

	// CRTP
	bool equalTo(const Team& other) const EGO_NOEXCEPT { return _teamID == other._teamID; }

    inline TEAM_REF toRef() const {return _teamID;}

private:
    TEAM_REF _teamID;                       ///< Unique team ID
    std::weak_ptr<Object> _leader;          ///< The leader of the team
    std::weak_ptr<Object> _sissy;           ///< Whoever called for help last
    std::array<bool, TEAM_MAX> _hatesTeam;  ///< Don't damage allies...
    uint16_t _morale;                       ///< Number of characters on team
};

inline bool VALID_TEAM_RANGE(const TEAM_REF team)
{
    return team < Team::TEAM_MAX;
}

bool team_hates_team(const Team& a, const Team& b);

bool team_hates_team(TEAM_REF a, TEAM_REF b);
