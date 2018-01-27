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

/// @file egolib/game/Logic/QuestLog.cpp
/// @author Zefz aka Johan Jansen

#include "egolib/game/Logic/QuestLog.hpp"

namespace Ego
{

QuestLog::QuestLog() :
    _questLog()
{
    //ctor
}

int QuestLog::getQuestProgress(const IDSZ2 &questID) const
{
    const auto& result = _questLog.find(questID);
    if(result == _questLog.end()) {
        return QUEST_NONE;
    }

    return result->second;
}

bool QuestLog::hasActiveQuest(const IDSZ2& questID) const
{
    int progress = getQuestProgress(questID);
    return progress != QUEST_NONE && progress != QUEST_BEATEN;
}

bool QuestLog::isBeaten(const IDSZ2& questID) const
{
    return getQuestProgress(questID) == QUEST_BEATEN;
}

int QuestLog::operator[] (const IDSZ2& questID) const
{
    return getQuestProgress(questID);
}

void QuestLog::setQuestProgress(const IDSZ2& questID, const int progress)
{
    _questLog[questID] = progress;
}

bool QuestLog::loadFromFile(const std::string& filePath)
{
    // blank out the existing map
    _questLog.clear();

    // Try to open a context
    std::unique_ptr<ReadContext> ctxt = nullptr;
    try {
        ctxt = std::make_unique<ReadContext>(filePath + "/quest.txt");
    } catch (...) {
        return false;
    }

    // Load each IDSZ
    while (ctxt->skipToColon(true))
    {
        IDSZ2 idsz = ctxt->readIDSZ();
        int  level = ctxt->readIntegerLiteral();

        // Try to add a single quest to the map
        _questLog[idsz] = level;
    }

    return true;
}

bool QuestLog::exportToFile(const std::string& filePath) const
{
    // Write a new quest file with all the quests
    vfs_FILE* filewrite = vfs_openWrite(filePath + "/quest.txt");
    if (!filewrite) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create quest file ", "`", filePath, "`", Log::EndOfEntry);
        return false;
    }

    vfs_printf(filewrite, "// This file keeps order of all the quests for this player\n");
    vfs_printf(filewrite, "// The number after the IDSZ shows the quest level. %i means it is completed.", QUEST_BEATEN);

    // Iterate through every element in the IDSZ map
    for(const auto& node : _questLog)
    {
        // Write every single quest to the quest log
        vfs_printf(filewrite, "\n:[%4s] %i", node.first.toString().c_str(), node.second);
    }

    // Clean up and return
    vfs_close(filewrite);
    return true;    
}

} //Ego