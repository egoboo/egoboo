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

/// @file egolib/Profiles/ModuleProfile.cpp
/// @author Johan Jansen

#define EGOLIB_PROFILES_PRIVATE 1
#include "egolib/Profiles/ModuleProfile.hpp"

#include "egolib/Core/StringUtilities.hpp"

#include "egolib/Log/_Include.hpp"

#include "egolib/vfs.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/platform.h"

const uint8_t ModuleProfile::RESPAWN_ANYTIME;
static const size_t SUMMARYLINES = 8;

ModuleProfile::ModuleProfile() :
    _loaded(false),
    _name("*UNKNOWN*"),
    _rank(0),
    _reference(),
    _importAmount(1),
    _allowExport(false),
    _minPlayers(0),
    _maxPlayers(0),
    _respawnValid(false),
    _summary(),
    _unlockQuest(IDSZ2::None),
    _unlockQuestLevel(-1), //-1 means none
    _moduleType(FILTER_SIDE_QUEST),
    _beaten(false),
    _icon(),
    _vfsPath(_name),
    _folderName(_name)
{}

ModuleProfile::~ModuleProfile()
{
}

bool ModuleProfile::isModuleUnlocked() const
{
    // First check if we are in developers mode or that the right module has been beaten before.
    if (egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        return true;
    }

    if (moduleHasIDSZ(_reference, _unlockQuest))
    {
        return true;
    }

//ZF> TODO: re-enable
/*
    if (base.importamount > 0)
    {
        // If that did not work, then check all selected players directories, but only if it isn't a starter module
        for(const std::shared_ptr<LoadPlayerElement> &player : _selectedPlayerList)
        {
            // find beaten quests or quests with proper level
            if(!player->hasQuest(base.unlockquest.id, base.unlockquest.level)) {
                return false;
            }
        }
    }
*/

    return true;
}

ModuleFilter ModuleProfile::getModuleType() const
{
    return _moduleType;
}


std::shared_ptr<ModuleProfile> ModuleProfile::loadFromFile(const std::string &folderPath)
{
    // see if we can open menu.txt file (required)
    ReadContext ctxt(folderPath + "/gamedat/menu.txt");

    //Allocate memory
    std::shared_ptr<ModuleProfile> result = std::make_shared<ModuleProfile>();

    std::string buffer;

    // Read basic data
    vfs_get_next_string_lit(ctxt, buffer);
    result->_name = buffer;

    vfs_get_next_name(ctxt, buffer);
    result->_reference = buffer;

    result->_unlockQuest = vfs_get_next_idsz(ctxt);
    ctxt.skipWhiteSpaces();
    if (!ctxt.isNewLine() && !ctxt.is(ReadContext::Traits::endOfInput()))
    {
        result->_unlockQuestLevel = ctxt.readIntegerLiteral();
    }
    result->_importAmount = vfs_get_next_int(ctxt);
    result->_allowExport  = vfs_get_next_bool(ctxt);
    result->_minPlayers = vfs_get_next_int(ctxt);
    result->_maxPlayers = vfs_get_next_int(ctxt);

    switch (vfs_get_next_printable(ctxt))
    {
        case 'T':
            result->_respawnValid = true;
        break;

        case 'A':
            result->_respawnValid = RESPAWN_ANYTIME;
        break;

        default:
            result->_respawnValid = false;
        break;
    }

    // Skip RTS option.
    vfs_get_next_printable(ctxt);

    vfs_get_next_string_lit(ctxt, buffer);
    buffer = Ego::trim_ws(buffer);
    result->_rank = buffer.length();

    // convert the special ranks of "unranked" or "-" ("rank 0")
    if ( '-' == buffer[0] || 'U' == Ego::toupper(buffer[0]) )
    {
        result->_rank = 0;
    }

    // Read the summary
    for (size_t cnt = 0; cnt < SUMMARYLINES; cnt++)
    {
        // load the string
        vfs_get_next_string_lit(ctxt, buffer);

        result->_summary.push_back(buffer);
    }

    // Assume default module type as a sidequest
    result->_moduleType = FILTER_SIDE_QUEST;

    // Read expansions
    while (ctxt.skipToColon(true))
    {
        IDSZ2 idsz = ctxt.readIDSZ();

        // Read module type
        if ( idsz == IDSZ2('T', 'Y', 'P', 'E') )
        {
            // parse the expansion value
            switch (Ego::toupper(ctxt.readPrintable()))
            {
                case 'M': result->_moduleType = FILTER_MAIN; break;
                case 'S': result->_moduleType = FILTER_SIDE_QUEST; break;
                case 'T': result->_moduleType = FILTER_TOWN; break;
                case 'F': result->_moduleType = FILTER_FUN; break;
                //case 'S': result->_moduleType = FILTER_STARTER; break;
            }
        }
        else if ( idsz == IDSZ2('B', 'E', 'A', 'T') )
        {
            result->_beaten = true;
        }
    }

    //Done!
    result->_loaded = true;

    // save the module path
    result->_vfsPath = folderPath;

    // load title image
    result->_icon = Ego::DeferredTexture(folderPath + "/gamedat/title");

    //Strip the prefix path and get just the folder name of the module
    result->_folderName = folderPath.substr(folderPath.find_last_of('/', folderPath.size() - 1) + 1);
                       /* folderPath.substr(folderPath.find_first_of('/') + 1); */

    return result;
}

bool ModuleProfile::moduleHasIDSZ(const std::string& szModName, const IDSZ2& idsz)
{
    /// @author ZZ
    /// @details This function returns true if the named module has the required IDSZ
    bool foundidsz;

    if ( idsz == IDSZ2::None ) return true;

    if ( szModName == "NONE" ) return false;

    std::string newLoadName = "mp_modules/" + szModName + "/gamedat/menu.txt";

    ReadContext ctxt(newLoadName);

    // Read basic data
    ctxt.skipToColon(false);  // Name of module...  Doesn't matter
    ctxt.skipToColon(false);  // Reference directory...
    ctxt.skipToColon(false);  // Reference IDSZ...
    ctxt.skipToColon(false);  // Import...
    ctxt.skipToColon(false);  // Export...
    ctxt.skipToColon(false);  // Min players...
    ctxt.skipToColon(false);  // Max players...
    ctxt.skipToColon(false);  // Respawn...
    ctxt.skipToColon(false);  // BAD! NOT USED
    ctxt.skipToColon(false);  // Rank...

    // Summary...
    for (size_t cnt = 0; cnt < SUMMARYLINES; cnt++)
    {
        ctxt.skipToColon(false);
    }

    // Now check expansions
    foundidsz = false;
    while (ctxt.skipToColon(true))
    {
        if ( ctxt.readIDSZ() == idsz )
        {
            foundidsz = true;
            break;
        }
    }

    return foundidsz;
}

bool ModuleProfile::moduleAddIDSZ(const std::string& szModName, const IDSZ2& idsz)
{
    /// @author ZZ
    /// @details This function appends an IDSZ to the module's menu.txt file

    vfs_FILE *filewrite;
    bool retval = false;

    // Only add if there isn't one already
    if ( !moduleHasIDSZ( szModName, idsz ) )
    {
        // make sure that the file exists in the user data directory since we are WRITING to it
        std::string source_file = "mp_modules/" + szModName + "/gamedat/menu.txt";
        std::string target_file = "/modules/" + szModName + "/gamedat/menu.txt";
        vfs_copyFile( source_file, target_file );

        // Try to open the file in append mode
        filewrite = vfs_openAppend(target_file);
        if ( NULL != filewrite )
        {
            // output the expansion IDSZ
            vfs_printf( filewrite, "\n:[%s]", idsz.toString().c_str() );

            // end the line
            vfs_printf( filewrite, "\n" );

            // success
            retval = true;

            // close the file
            vfs_close( filewrite );
        }
    }

    return retval;
}
