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

#include "Team.hpp"
#include "game/Entities/_include.hpp"

std::array<Team, TEAM_MAX> TeamStack;

void reset_teams()
{
    /// @author ZZ
    /// @details This function makes everyone hate everyone else

    TEAM_REF teama, teamb;

    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        // Make the team hate everyone
        for ( teamb = 0; teamb < TEAM_MAX; teamb++ )
        {
            TeamStack[teama].hatesteam[REF_TO_INT( teamb )] = true;
        }

        // Make the team like itself
        TeamStack[teama].hatesteam[REF_TO_INT( teama )] = false;

        // Set defaults
        TeamStack[teama]._leader.reset();
        TeamStack[teama].sissy = 0;
        TeamStack[teama].morale = 0;
    }

    // Keep the null team neutral
    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        TeamStack[teama].hatesteam[TEAM_NULL] = false;
        TeamStack[( TEAM_REF )TEAM_NULL].hatesteam[REF_TO_INT( teama )] = false;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( const TEAM_REF team, int amount, XPType xptype )
{
    /// @author ZZ
    /// @details This function gives every character on a team experience

    for(const std::shared_ptr<Object> &chr : _gameObjects.iterator())
    {
        if ( chr->team == team )
        {
            chr->giveExperience(amount, xptype, false);
        }
    }
}

//--------------------------------------------------------------------------------------------
bool team_hates_team( const TEAM_REF ipredator_team, const TEAM_REF iprey_team )
{
    /// @author BB
    /// @details a wrapper function for access to the hatesteam data

    if ( ipredator_team >= TEAM_MAX || iprey_team >= TEAM_MAX ) return false;

    return TeamStack[ipredator_team].hatesteam[ REF_TO_INT( iprey_team )];
}

std::shared_ptr<Object> Team::getLeader() const
{
	return _leader.lock();
}

void Team::setLeader(const std::shared_ptr<Object> &object)
{
	_leader = object;
}