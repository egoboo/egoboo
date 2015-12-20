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
    _icon(std::make_shared<Ego::OpenGL::Texture>()),
    _vfsPath(_name),
    _folderName(_name)
{}

ModuleProfile::~ModuleProfile()
{
    _icon = nullptr;
}

bool ModuleProfile::isModuleUnlocked() const
{
    // First check if we are in developers mode or that the right module has been beaten before.
    if (egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        return true;
    }

    if (moduleHasIDSZ(_reference.c_str(), _unlockQuest, 0, nullptr))
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
    STRING buffer;

    // see if we can open menu.txt file (required)
    ReadContext ctxt(folderPath + "/gamedat/menu.txt");
    if (!ctxt.ensureOpen())
    {
        return nullptr;
    }

    //Allocate memory
    std::shared_ptr<ModuleProfile> result = std::make_shared<ModuleProfile>();

    // Read basic data
    vfs_get_next_string_lit(ctxt, buffer, SDL_arraysize(buffer));
    result->_name = buffer;

    vfs_get_next_name(ctxt, buffer, SDL_arraysize(buffer));
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

    vfs_get_next_string_lit(ctxt, buffer, SDL_arraysize(buffer));
    str_trim(buffer);
    result->_rank = strlen(buffer);

    // convert the special ranks of "unranked" or "-" ("rank 0")
    if ( '-' == buffer[0] || 'U' == Ego::toupper(buffer[0]) )
    {
        result->_rank = 0;
    }

    // Read the summary
    for (size_t cnt = 0; cnt < SUMMARYLINES; cnt++)
    {
        // load the string
        vfs_get_next_string_lit(ctxt, buffer, SDL_arraysize(buffer));

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
    ctxt.close();

    // save the module path
    result->_vfsPath = folderPath;

    /// @note just because we can't load the title image DOES NOT mean that we ignore the module
    // load title image
    ego_texture_load_vfs(result->_icon, (folderPath + "/gamedat/title").c_str());

    /// @note This is kinda a cheat since we know that the virtual paths all begin with "mp_" at the moment.
    // If that changes, this line must be changed as well.
    result->_folderName = folderPath.substr(11);

    return result;
}

bool ModuleProfile::moduleHasIDSZ(const char *szModName, const IDSZ2& idsz, size_t buffer_len, char * buffer)
{
    /// @author ZZ
    /// @details This function returns true if the named module has the required IDSZ
    bool foundidsz;

    if ( idsz == IDSZ2::None ) return true;

    if ( 0 == strcmp( szModName, "NONE" ) ) return false;

    std::string newLoadName = "mp_modules/" + std::string(szModName) + "/gamedat/menu.txt";

    ReadContext ctxt(newLoadName);
    if (!ctxt.ensureOpen())
    {
        return false;
    }
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

    if (buffer)
    {
        if (buffer_len < 1 )
        {
            /* nothing */
        }
        else if (1 == buffer_len)
        {
            buffer[0] = CSTR_END;
        }
        else
        {
            vfs_read_string_lit(ctxt, buffer, buffer_len);
        }
    }

    return foundidsz;
}

bool ModuleProfile::moduleAddIDSZ(const char *szModName, const IDSZ2& idsz, size_t buffer_len, const char * buffer)
{
    /// @author ZZ
    /// @details This function appends an IDSZ to the module's menu.txt file

    vfs_FILE *filewrite;
    bool retval = false;

    // Only add if there isn't one already
    if ( !moduleHasIDSZ( szModName, idsz, 0, NULL ) )
    {
        // make sure that the file exists in the user data directory since we are WRITING to it
        std::string src_file = std::string("mp_modules/") + szModName + "/gamedat/menu.txt";
        std::string dst_file = std::string("/modules/") + szModName + "/gamedat/menu.txt";
        vfs_copyFile( src_file, dst_file );

        // Try to open the file in append mode
        filewrite = vfs_openAppend(dst_file);
        if ( NULL != filewrite )
        {
            // output the expansion IDSZ
            vfs_printf( filewrite, "\n:[%s]", idsz.toString().c_str() );

            // output an optional parameter
            if ( NULL != buffer && buffer_len > 1 )
            {
                vfs_printf( filewrite, " %s", idsz.toString().c_str() );
            }

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
