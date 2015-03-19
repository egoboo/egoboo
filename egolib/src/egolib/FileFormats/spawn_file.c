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

#include "egolib/_math.h"

// includes for egoboo constants
#include "game/char.h"       // for TEAM_* constants

spawn_file_info_t::spawn_file_info_t() :
    do_spawn(false),
    spawn_coment(),
    spawn_name(),
    pname(nullptr),
    slot(-1),
    pos(0, 0, 0),
    passage(-1),
    content(0),
    money(0),
    level(0),
    skin(0),
    stat(false),
    team(TEAM_NULL),
    facing(FACE_NORTH),
    attach(ATTACH_NONE),
    parent(INVALID_CHR_REF)   
{
    //ctor
}

spawn_file_info_t *spawn_file_info_init(spawn_file_info_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    BLANK_STRUCT_PTR(self);
    self->attach = ATTACH_NONE;
    self->team = TEAM_NULL;
    return self;
}

spawn_file_info_t *spawn_file_info_reinit(spawn_file_info_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    // Save the parent.
    CHR_REF parent = self->parent;
    // Reset the data.
    spawn_file_info_init(self);
    // Restore the parent.
    self->parent = parent;
    return self;
}

bool spawn_file_read(ReadContext& ctxt, spawn_file_info_t *info)
{
    if (!info)
    {
        return false;
    }
    spawn_file_info_reinit(info);
Again:
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
        } while (!ctxt.is(':') && !ctxt.isNewLine() && !ctxt.is(ReadContext::EndOfInput) && !ctxt.is(ReadContext::Error));
        if (ctxt.is(ReadContext::Error))
        {
            throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber));
        }
        if (ctxt.is(ReadContext::EndOfInput))
        {
            return false;
        }
        if (!ctxt.is(':'))
        {
            ctxt.readToEndOfLine();
            goto Again;
        }
        ctxt.next();
        std::string name = Ego::trim(ctxt._buffer.toString());

        
        strncpy(info->spawn_coment, name.c_str(), SDL_arraysize(info->spawn_coment));
        info->do_spawn = true;

        vfs_read_string_lit(ctxt, info->spawn_name, SDL_arraysize(info->spawn_name));

        info->pname = info->spawn_name;
        if (!strcmp(info->spawn_name, "NONE"))
        {
            // A random name is selected.
            info->pname = nullptr;
        }

        info->slot = ctxt.readInt();

        info->pos.x = ctxt.readReal() * GRID_FSIZE;
        info->pos.y = ctxt.readReal() * GRID_FSIZE;
        info->pos.z = ctxt.readReal() * GRID_FSIZE;

        info->facing = FACE_NORTH;
        info->attach = ATTACH_NONE;
        char chr = ctxt.readPrintable();
        switch (Ego::toupper(chr))
        {
        case 'S': info->facing = FACE_SOUTH;       break;
        case 'E': info->facing = FACE_EAST;        break;
        case 'W': info->facing = FACE_WEST;        break;
        case 'N': info->facing = FACE_NORTH;       break;
        case '?': info->facing = FACE_RANDOM;      break;
        case 'L': info->attach = ATTACH_LEFT;      break;
        case 'R': info->attach = ATTACH_RIGHT;     break;
        case 'I': info->attach = ATTACH_INVENTORY; break;
        default:
        {
            throw Ego::Script::SyntaxError(__FILE__, __LINE__, Ego::Script::Location(ctxt.getLoadName(), ctxt.getLineNumber()));
        }
        };
        info->money = ctxt.readInt();
        info->skin = ctxt.readInt();
        info->passage = ctxt.readInt();
        info->content = ctxt.readInt();
        info->level = ctxt.readInt();

        if (info->skin >= MAX_SKIN)
        {
            int irand = Random::next(std::numeric_limits<uint16_t>::max());
            info->skin = irand % MAX_SKIN;     // Randomize skin?
        }

        info->stat = ctxt.readBool();

        ctxt.readPrintable();   ///< BAD! Unused ghost value

        chr = ctxt.readPrintable();
        info->team = (chr - 'A') % TEAM_MAX;
        
        return true;
    }
    else if (ctxt.is('#'))
    {
        ctxt.next();
        info->do_spawn = false;

        std::string what = ctxt.readName();
        if (what != "dependency")
        {
            throw Ego::Script::SyntaxError(__FILE__,__LINE__,Ego::Script::Location(ctxt._loadName,ctxt._lineNumber));
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
            throw Ego::Script::SyntaxError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber));
        }
        if (who.length() >= SDL_arraysize(info->spawn_coment))
        {
            throw Ego::Script::SyntaxError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber));
        }
        int slot = ctxt.readInt();
        // Store the data.
        strncpy(info->spawn_coment, who.c_str(), SDL_arraysize(info->spawn_coment));
        info->slot = slot;
        return true;
    }
    else if (!ctxt.is(ReadContext::EndOfInput))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber));
    }
    return false;
}


