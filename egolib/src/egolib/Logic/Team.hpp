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
#include "game/egoboo_typedef.h"

enum TeamTypes : uint8_t
{
    TEAM_EVIL            = ( 'E' - 'A' ),        ///< Evil team
    TEAM_GOOD            = ( 'G' - 'A' ),        ///< Good team
    TEAM_NULL            = ( 'N' - 'A' ),        ///< Null or Neutral team
    TEAM_ZIPPY           = ( 'Z' - 'A' ),        ///< Zippy Team?
    TEAM_DAMAGE,                                 ///< For damage tiles
    TEAM_MAX
};

/// The description of a single team
class Team
{
    //TODO: make private
public:
    bool hatesteam[TEAM_MAX];    	 ///< Don't damage allies...
    uint16_t morale;                 ///< Number of characters on team
    CHR_REF  sissy;                  ///< Whoever called for help last

public:
	std::shared_ptr<Object> getLeader() const;
	void setLeader(const std::shared_ptr<Object> &object);

public:
    std::weak_ptr<Object> _leader;	///< The leader of the team
};


extern std::array<Team, TEAM_MAX> TeamStack;

#define VALID_TEAM_RANGE( ITEAM ) ( ((ITEAM) >= 0) && ((ITEAM) < TEAM_MAX) )

//Function prototypes
void reset_teams();
void give_team_experience( const TEAM_REF team, int amount, XPType xptype );
bool team_hates_team( const TEAM_REF ipredator_team, const TEAM_REF iprey_team );
