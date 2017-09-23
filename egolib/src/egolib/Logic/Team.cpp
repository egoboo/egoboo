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
#include "egolib/Entities/_Include.hpp"
#include "game/Module/Module.hpp"

Team::Team(const TEAM_REF teamID) :
    _teamID(teamID),
    _leader(),
    _sissy(),
    _hatesTeam{},
    _morale(0)
{

    // Make the team hate everyone else
    if(_teamID != TEAM_NULL) {
        _hatesTeam.fill(true);

        //keep the null team neutral
        _hatesTeam[TEAM_NULL] = false;

        //Make the team like itself
        _hatesTeam[_teamID] = false;
    }
    else {
        //TEAM_NULL likes everybody
        _hatesTeam.fill(false);
    }

}

void Team::giveTeamExperience(const int amount, const XPType xptype) const
{
    for(const std::shared_ptr<Object> &chr : _currentModule->getObjectHandler().iterator())
    {
        if ( chr->getTeam()._teamID == _teamID )
        {
            chr->giveExperience(amount, xptype, false);
        }
    }
}

bool Team::hatesTeam(const Team &other) const
{
    return _hatesTeam[other._teamID];
}

std::shared_ptr<Object> Team::getLeader() const
{
	return _leader.lock();
}

void Team::setLeader(const std::shared_ptr<Object> &object)
{
	_leader = object;
}

void Team::callForHelp(const std::shared_ptr<Object> &caller)
{
    _sissy = caller;

    //Notify all other characters who are friendly that this character has called for help
    for(const std::shared_ptr<Object> &chr : _currentModule->getObjectHandler().iterator())
    {
        if ( chr != caller && !chr->getTeam().hatesTeam(caller->getTeam()) )
        {
            SET_BIT( chr->ai.alert, ALERTIF_CALLEDFORHELP );
        }
    }
}

std::shared_ptr<Object> Team::getSissy() const
{
    return _sissy.lock();
}

void Team::makeAlliance(const Team &other)
{
    _hatesTeam[other._teamID] = false;
}

uint16_t Team::getMorale() const
{
    return _morale;
}

void Team::increaseMorale()
{
    _morale++;
}

void Team::decreaseMorale()
{
    if(_morale > 0)
    {
        _morale--;
    }
}

bool team_hates_team(const Team& a, const Team& b) {
    return a.hatesTeam(b);
}

bool team_hates_team(TEAM_REF a, TEAM_REF b) {
    return team_hates_team(_currentModule->getTeamList()[a], _currentModule->getTeamList()[b]);
}
