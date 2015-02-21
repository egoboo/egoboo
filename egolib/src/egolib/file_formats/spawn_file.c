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

/// @file egolib/file_formats/spawn_file.c
/// @brief Implementation of a scanner for Egoboo's spawn.txt file
/// @details

#include "egolib/file_formats/spawn_file.h"

#include "egolib/fileutil.h"
#include "egolib/strutil.h"

#include "egolib/_math.h"

// includes for egoboo constants
#include "game/char.h"       // for TEAM_* constants

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
spawn_file_info_t * spawn_file_info_init( spawn_file_info_t *pinfo )
{
    /// @author BB
    /// @details safe values for all parameters

    if ( NULL == pinfo ) return pinfo;

    BLANK_STRUCT_PTR( pinfo )

    pinfo->attach = ATTACH_NONE;
    pinfo->team   = TEAM_NULL;

    return pinfo;
}

//--------------------------------------------------------------------------------------------
spawn_file_info_t * spawn_file_info_reinit( spawn_file_info_t *pinfo )
{
    CHR_REF old_parent;

    if ( NULL == pinfo ) return pinfo;

    // save the parent data just in case
    old_parent = pinfo->parent;

    // init the data
    spawn_file_info_init( pinfo );

    // restore the parent data
    pinfo->parent = old_parent;

    return pinfo;
}

//--------------------------------------------------------------------------------------------

bool spawn_file_scan(ReadContext& ctxt, spawn_file_info_t *pinfo)
{
    // trap bad pointers
    if (!pinfo ) return false;

    spawn_file_info_reinit( pinfo );
Again:
    // Until we hit something else than newlines, whitespaces or comments.
    while (true)
    {
        ctxt.skipWhiteSpaces();
        while (ctxt.isNewLine()) /// @todo Add and use ReadContext::skipNewLines().
        {
            ctxt.next();
            ctxt._lineNumber++;
            continue;
        }
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
    if (ctxt.isAlpha()||ctxt.is('%'))
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
        std::string name = trim(ctxt._buffer.toString());

        
        strncpy(pinfo->spawn_coment, name.c_str(), SDL_arraysize(pinfo->spawn_coment));
        pinfo->do_spawn = true;

        vfs_read_string_lit(ctxt, pinfo->spawn_name, SDL_arraysize(pinfo->spawn_name));

        pinfo->pname = pinfo->spawn_name;
        if ( 0 == strcmp( pinfo->spawn_name, "NONE" ) )
        {
            // Random pinfo->pname
            pinfo->pname = NULL;
        }

        pinfo->slot = ctxt.readInt();

        pinfo->pos.x = ctxt.readReal() * GRID_FSIZE;
        pinfo->pos.y = ctxt.readReal() * GRID_FSIZE;
        pinfo->pos.z = ctxt.readReal() * GRID_FSIZE;

        pinfo->facing = FACE_NORTH;
        pinfo->attach = ATTACH_NONE;
        char chr = ctxt.readPrintable();
        switch (char_toupper(chr))
        {
        case 'S': pinfo->facing = FACE_SOUTH;       break;
        case 'E': pinfo->facing = FACE_EAST;        break;
        case 'W': pinfo->facing = FACE_WEST;        break;
        case 'N': pinfo->facing = FACE_NORTH;       break;
        case '?': pinfo->facing = FACE_RANDOM;      break;
        case 'L': pinfo->attach = ATTACH_LEFT;      break;
        case 'R': pinfo->attach = ATTACH_RIGHT;     break;
        case 'I': pinfo->attach = ATTACH_INVENTORY; break;
        default:
        {
            throw Ego::Script::SyntaxError(__FILE__, __LINE__, Ego::Script::Location(ctxt.getLoadName(), ctxt.getLineNumber()));
        }
        };
        pinfo->money   = ctxt.readInt();
        pinfo->skin    = ctxt.readInt();
        pinfo->passage = ctxt.readInt();
        pinfo->content = ctxt.readInt();
        pinfo->level   = ctxt.readInt();

        if ( pinfo->skin >= MAX_SKIN )
        {
            int irand = RANDIE;
            pinfo->skin = irand % MAX_SKIN;     // Randomize skin?
        }

        pinfo->stat = ctxt.readBool();

        ctxt.readPrintable();   ///< BAD! Unused ghost value

        chr = ctxt.readPrintable();
        pinfo->team = (chr - 'A' ) % TEAM_MAX;
        
        return true;
    }
    else if (ctxt.is('#'))
    {
        ctxt.next();
        pinfo->do_spawn = false;

        std::string what = ctxt.readName();
        if (what != "dependency")
        {
            throw Ego::Script::SyntaxError(__FILE__,__LINE__,Ego::Script::Location(ctxt._loadName,ctxt._lineNumber));
        }
        std::string who = ctxt.readName();
        if (who.empty()) /// @todo Verify that this is unnecessary based on the definition of readName.
        {
            throw Ego::Script::SyntaxError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber));
        }
        if (who.length() >= SDL_arraysize(pinfo->spawn_coment))
        {
            throw Ego::Script::SyntaxError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber));
        }
        int slot = ctxt.readInt();
        // Store the data.
        strncpy(pinfo->spawn_coment, who.c_str(), SDL_arraysize(pinfo->spawn_coment));
        pinfo->slot = slot;
        return true;
    }
    else if (!ctxt.is(ReadContext::EndOfInput))
    {
        throw Ego::Script::LexicalError(__FILE__, __LINE__, Ego::Script::Location(ctxt._loadName, ctxt._lineNumber));
    }
    return false;
}


