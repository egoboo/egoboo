#include "egolib/FileFormats/SpawnFile/SpawnFileReaderImpl.hpp"
#include "egolib/fileutil.h"
#include "egolib/FileFormats/SpawnFile/spawn_file.h"
#include "egolib/strutil.h"
#include "egolib/Logic/Team.hpp"

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
        ctxt.skip_new_lines();
        if (ctxt.is('/'))
        {
            ctxt.readSingleLineComment(); /// @todo Add and use ReadContext::skipSingleLineComment().
            continue;
        }
        if (!ctxt.is_white_space() && !ctxt.is_new_line() && !ctxt.is('/'))
        {
            break;
        }
    }
    if (ctxt.is_alpha() || ctxt.is('%') || ctxt.is('_'))
    {
        ctxt.clear_lexeme_text();
        // Read everything into the buffer until a ':', a new line, an error or the end of the input is reached.
        do
        {
            ctxt.save_and_next();
        } while (!ctxt.is(':') && !ctxt.is_new_line() && !ctxt.is_end_of_input() && !ctxt.is_error());
        if (ctxt.is_error())
        {
            throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, ctxt.get_location(),
                                        "read error");
        }
        if (ctxt.is_end_of_input())
        {
            return false;
        }
        if (!ctxt.is(':'))
        {
            throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, ctxt.get_location(),
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

        info.facing = Facing::FACE_NORTH;
        info.attach = ATTACH_NONE;
        char chr = ctxt.readPrintable();
        switch (id::to_upper(chr))
        {
            case 'S': info.facing = Facing::FACE_SOUTH;       break;
            case 'E': info.facing = Facing::FACE_EAST;        break;
            case 'W': info.facing = Facing::FACE_WEST;        break;
            case 'N': info.facing = Facing::FACE_NORTH;       break;
            case '?': info.facing = Facing(FACE_RANDOM);      break;
            case 'L': info.attach = ATTACH_LEFT;      break;
            case 'R': info.attach = ATTACH_RIGHT;     break;
            case 'I': info.attach = ATTACH_INVENTORY; break;
            default:
            {
                throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::syntactical, ctxt.get_location(),
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
            throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::syntactical, ctxt.get_location(),
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
            throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::syntactical, ctxt.get_location(),
                                        "syntax error");
        }
        int slot = ctxt.readIntegerLiteral();
        // Store the data.
        info.spawn_comment = who;
        info.slot = slot;
        return true;
    }
    else if (!ctxt.is_end_of_input())
    {
        throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, ctxt.get_location(),
                                    "junk after end of spawn file");
    }
    return false;
}

std::vector<spawn_file_info_t> SpawnFileReaderImpl::read(const std::string& pathname)
{
    ReadContext ctxt(pathname);
    ctxt.next(); /// @todo Remove this hack.
    std::vector<spawn_file_info_t> entries;
    while (!ctxt.is_end_of_input())
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
