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

/// @file egolib/FileFormats/spawn_file.c
/// @brief Implementation of a scanner for Egoboo's spawn.txt file
/// @details

#include "egolib/FileFormats/spawn_file.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/_math.h"
#include "egolib/Logic/Team.hpp"

spawn_file_info_t::spawn_file_info_t() :
    do_spawn(false),
    spawn_comment(),
    spawn_name(),
    pname(nullptr),
    slot(-1),
    pos(0.0f, 0.0f, 0.0f),
    passage(-1),
    content(0),
    money(0),
    level(0),
    skin(0),
    stat(false),
    team(Team::TEAM_NULL),
    facing(Facing::FACE_NORTH),
    attach(ATTACH_NONE)
{
    //ctor
}

void spawn_file_info_t::init(spawn_file_info_t& self)
{
	self = spawn_file_info_t();
}

void spawn_file_info_t::reinit(spawn_file_info_t& self)
{
    // Reset the data.
    init(self);
}

bool spawn_file_read(ReadContext& ctxt, spawn_file_info_t& info)
{
    spawn_file_info_t::reinit(info);

    // Until we hit something else than newlines, whitespaces or comments.
    while (true)
    {
        ctxt.skipWhiteSpaces();
        ctxt.skipNewLines();
        if (ctxt.is('/'))
        {
            ctxt.readSingleLineComment(); /// @todo Add and use ReadContext::skipSingleLineComment().
            continue;
        }
        if (!ctxt.isWhiteSpace() && !ctxt.isNewLine() && !ctxt.is('/'))
        {
            break;
        }
    }
    if (ctxt.isAlpha()||ctxt.is('%')||ctxt.is('_'))
    {
        ctxt._buffer.clear();
        // Read everything into the buffer until a ':', a new line, an error or the end of the input is reached.
        do
        {
            ctxt.saveAndNext();
        } while (!ctxt.is(':') && !ctxt.isNewLine() && !ctxt.is(ReadContext::Traits::endOfInput()) &&
                 !ctxt.is(ReadContext::Traits::error()));
        if (ctxt.is(ReadContext::Traits::error()))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber),
                                            "read error");
        }
        if (ctxt.is(ReadContext::Traits::endOfInput()))
        {
            return false;
        }
        if (!ctxt.is(':'))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber),
                                            "expected `:`");
        }
        ctxt.next();
        std::string name = Ego::trim(ctxt._buffer.toString());

        
        strncpy(info.spawn_comment, name.c_str(), SDL_arraysize(info.spawn_comment));
        info.do_spawn = true;

        vfs_read_string_lit(ctxt, info.spawn_name, SDL_arraysize(info.spawn_name));

        info.pname = info.spawn_name;
        if (!strcmp(info.spawn_name, "NONE"))
        {
            // A random name is selected.
            info.pname = nullptr;
        }

        info.slot = ctxt.readIntegerLiteral();

        info.pos[kX] = ctxt.readRealLiteral() * Info<float>::Grid::Size();
        info.pos[kY] = ctxt.readRealLiteral() * Info<float>::Grid::Size();
        info.pos[kZ] = ctxt.readRealLiteral() * Info<float>::Grid::Size();

        info.facing = FACING_T(Facing::FACE_NORTH);
        info.attach = ATTACH_NONE;
        char chr = ctxt.readPrintable();
        switch (Ego::toupper(chr))
        {
            case 'S': info.facing = FACING_T(Facing::FACE_SOUTH);       break;
            case 'E': info.facing = FACING_T(Facing::FACE_EAST);        break;
            case 'W': info.facing = FACING_T(Facing::FACE_WEST);        break;
            case 'N': info.facing = FACING_T(Facing::FACE_NORTH);       break;
            case '?': info.facing = FACE_RANDOM;      break;
            case 'L': info.attach = ATTACH_LEFT;      break;
            case 'R': info.attach = ATTACH_RIGHT;     break;
            case 'I': info.attach = ATTACH_INVENTORY; break;
            default:
            {
                throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt.getLoadName(), ctxt.getLineNumber()),
                                                    "invalid enumeration element");
            }
        };
        info.money = ctxt.readIntegerLiteral();

        //If the skin type is a '?' character then it means random skin else it's an integer
        ctxt.skipWhiteSpaces();
        if(ctxt.is('?')) {
            info.skin = ObjectProfile::NO_SKIN_OVERRIDE;
            ctxt.next();
        }
        else {
            info.skin = ctxt.readIntegerLiteral();
        }

        info.passage = ctxt.readIntegerLiteral();
        info.content = ctxt.readIntegerLiteral();
        info.level = ctxt.readIntegerLiteral();
        info.stat = ctxt.readBool();

        ctxt.readPrintable();   ///< BAD! Unused ghost value

        chr = ctxt.readPrintable();
        info.team = (chr - 'A') % Team::TEAM_MAX;
        
        return true;
    }
    else if (ctxt.is('#'))
    {
        ctxt.next();
        info.do_spawn = false;

        std::string what = ctxt.readName();
        if (what != "dependency")
        {
            throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt._loadName,ctxt._lineNumber),
                                                "syntax error");
        }
        std::string who;
        ctxt.skipWhiteSpaces();
        if (ctxt.is('%'))
        {
            who = ctxt.readReference();
        }
        else
        {
            who = ctxt.readName();
        }
        if (who.empty()) /// @todo Verify that this is unnecessary based on the definition of readName.
        {
            throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt._loadName, ctxt._lineNumber),
                                                "syntax error");
        }
        if (who.length() >= SDL_arraysize(info.spawn_comment))
        {
            throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt._loadName, ctxt._lineNumber),
                                                "syntax error");
        }
        int slot = ctxt.readIntegerLiteral();
        // Store the data.
        strncpy(info.spawn_comment, who.c_str(), SDL_arraysize(info.spawn_comment));
        info.slot = slot;
        return true;
    }
    else if (!ctxt.is(ReadContext::Traits::endOfInput()))
    {
        throw Id::LexicalErrorException(__FILE__, __LINE__, Id::Location(ctxt._loadName, ctxt._lineNumber),
                                        "junk after end of spawn file");
    }
    return false;
}


