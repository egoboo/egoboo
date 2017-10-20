#include "egolib/FileFormats/SpawnFile/SpawnFileReaderImpl.hpp"
#include "egolib/fileutil.h"
#include "egolib/FileFormats/SpawnFile/spawn_file.h"
#include "egolib/strutil.h"
#include "egolib/Logic/Team.hpp"

#pragma push_macro("ERROR")
#undef ERROR

SpawnFileReaderImpl::SpawnFileReaderImpl()
{}

SpawnFileReaderImpl::~SpawnFileReaderImpl()
{}

bool SpawnFileReaderImpl::read(ReadContext& ctxt, spawn_file_info_t& info)
{
    info = spawn_file_info_t();

    // Until we hit something else than newlines, whitespaces or comments.
    while (true)
    {
        ctxt.skipWhiteSpaces();
        ctxt.new_lines(nullptr);
        if (ctxt.is('/'))
        {
            ctxt.readSingleLineComment(); /// @todo Add and use ReadContext::skipSingleLineComment().
            continue;
        }
        if (!ctxt.ise(ctxt.WHITE_SPACE()) && !ctxt.ise(ctxt.NEW_LINE()) && !ctxt.is('/'))
        {
            break;
        }
    }
    if (ctxt.ise(ctxt.ALPHA()) || ctxt.is('%') || ctxt.is('_'))
    {
        ctxt.clear_lexeme_text();
        // Read everything into the buffer until a ':', a new line, an error or the end of the input is reached.
        do
        {
            ctxt.save_and_next();
        } while (!ctxt.is(':') && !ctxt.ise(ctxt.NEW_LINE()) && !ctxt.ise(ctxt.END_OF_INPUT()) && !ctxt.ise(ctxt.ERROR()));
        if (ctxt.ise(ctxt.ERROR()))
        {
            throw id::c::compilation_error(__FILE__, __LINE__, id::c::compilation_error_kind::lexical, ctxt.get_location(),
                                           "read error");
        }
        if (ctxt.ise(ctxt.END_OF_INPUT()))
        {
            return false;
        }
        if (!ctxt.is(':'))
        {
            throw id::c::compilation_error(__FILE__, __LINE__, id::c::compilation_error_kind::lexical, ctxt.get_location(),
                                           "expected `:`");
        }
        ctxt.next();

        info.spawn_comment = Ego::trim_ws(ctxt.get_lexeme_text());

        info.do_spawn = true;

        info.spawn_name = vfs_read_string_lit(ctxt);

        info.slot = ctxt.readIntegerLiteral();

        info.pos[kX] = ctxt.readRealLiteral() * Info<float>::Grid::Size();
        info.pos[kY] = ctxt.readRealLiteral() * Info<float>::Grid::Size();
        info.pos[kZ] = ctxt.readRealLiteral() * Info<float>::Grid::Size();

        info.facing = FACE_NORTH;
        info.attach = ATTACH_NONE;
        char chr = ctxt.readPrintable();
        switch (id::to_upper(chr))
        {
            case 'S': info.facing = FACE_SOUTH;       break;
            case 'E': info.facing = FACE_EAST;        break;
            case 'W': info.facing = FACE_WEST;        break;
            case 'N': info.facing = FACE_NORTH;       break;
			case '?': info.facing = Facing::random(); break;
            case 'L': info.attach = ATTACH_LEFT;      break;
            case 'R': info.attach = ATTACH_RIGHT;     break;
            case 'I': info.attach = ATTACH_INVENTORY; break;
            default:
            {
                throw id::c::compilation_error(__FILE__, __LINE__, id::c::compilation_error_kind::syntactical, ctxt.get_location(),
                                               "invalid enumeration element");
            }
        };
        info.money = ctxt.readIntegerLiteral();

        //If the skin type is a '?' character then it means random skin else it's an integer
        ctxt.skipWhiteSpaces();
        if (ctxt.is('?'))
        {
            info.skin = ObjectProfile::NO_SKIN_OVERRIDE;
            ctxt.next();
        }
        else
        {
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
            throw id::c::compilation_error(__FILE__, __LINE__, id::c::compilation_error_kind::syntactical, ctxt.get_location(),
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
            throw id::c::compilation_error(__FILE__, __LINE__, id::c::compilation_error_kind::syntactical, ctxt.get_location(),
                                           "syntax error");
        }
        int slot = ctxt.readIntegerLiteral();
        // Store the data.
        info.spawn_comment = who;
        info.slot = slot;
        return true;
    }
    else if (!ctxt.ise(ctxt.END_OF_INPUT()))
    {
        throw id::c::compilation_error(__FILE__, __LINE__, id::c::compilation_error_kind::lexical, ctxt.get_location(),
                                       "junk after end of spawn file");
    }
    return false;
}

std::vector<spawn_file_info_t> SpawnFileReaderImpl::read(const std::string& pathname)
{
    ReadContext ctxt(pathname);
    std::vector<spawn_file_info_t> entries;
    while (!ctxt.ise(ctxt.END_OF_INPUT()))
    {
        spawn_file_info_t entry;
        // Read next entry.
        if (!read(ctxt, entry))
        {
            break; // No more entries.
        }
        // Add entry.
        entries.push_back(entry);
    }
    return entries;
}

#pragma pop_macro("ERROR")
