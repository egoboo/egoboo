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

/// @file egolib/game/Logic/QuestLog.hpp
/// @author Zefz aka Johan Jansen

#pragma once

#include "idlib/idlib.hpp"
#include "egolib/egolib.h"

namespace Ego
{

class QuestLog
{
public:
    static constexpr int QUEST_NONE = -1;
    static constexpr int QUEST_BEATEN = -2;

    QuestLog();

    /**
    * @brief
    *   read access to the progress of a quest.
    *   Returns QUEST_NONE if it does not exist or QUEST_BEATEN if
    *   it has been beaten.
    **/
    int operator[] (const IDSZ2& questID) const;

    /**
    * @brief
    *   Changes the progress of a given quest.
    *   This function can also remove the quest by setting QUEST_NONE
    *   or marking it as beaten by QUEST_BEATEN.
    **/
    void setQuestProgress(const IDSZ2& questID, const int progress);

    /**
    * @return
    *   true if the specified quest has been beaten
    **/
    bool isBeaten(const IDSZ2& questID) const;

    /**
    * @return
    *   true if this quest log contains the given quest and it has
    *   not been beaten yet
    **/
    bool hasActiveQuest(const IDSZ2& questID) const;

    /**
    * @brief
    *   Opens and parses a quest.txt file and loads it into
    *   this QuestLog.
    * @param filePath
    *   The path of the quest.txt file
    * @return
    *   true if the file was found and successfully parsed
    **/
    bool loadFromFile(const std::string& filePath);

    /**
    * @brief
    *   This exports quest log data into a quest.txt file
    * @param filePath
    *   The path of the quest.txt file
    * @return
    *   true if the quest data was successfully stored into
    *   the specified file
    **/
    bool exportToFile(const std::string& filePath) const;

private:
    int getQuestProgress(const IDSZ2 &questID) const;

private:
    std::unordered_map<IDSZ2, int> _questLog;
};

} //Ego