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

/// @file egolib/fileutil.c
/// @brief Implementation of Egoboo file utilities
/// @details

#include "egolib/fileutil.h"

#include "egolib/Script/DDLTokenDecoder.hpp"
#include "egolib/Log/_Include.hpp"
#include "egolib/strutil.h"
#include "egolib/platform.h"
#include "egolib/egoboo_setup.h"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Image/ImageLoader.hpp"
#include "egolib/_math.h"
// includes for egoboo constants
#include "egolib/Graphics/ModelDescriptor.hpp"                    // for ACTION_* constants

#pragma push_macro("ERROR")
#undef ERROR

ReadContext::ReadContext(const std::string& fileName) :
    Scanner(fileName)
{
}

ReadContext::~ReadContext()
{
}

float ReadContext::toReal() const
{
    float temporary;
    auto lexeme = get_lexeme_text();
    if (!idlib::hll::decoder<float>()(lexeme,temporary))
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                            "unable to convert current lexeme `" + lexeme + "` into a value of type "
                                            "`float`");
    }
    return temporary;
}

void ReadContext::skipWhiteSpaces()
{
    if (ise(ERROR()))
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                            "read error");
    }
    if (ise(END_OF_INPUT()))
    {
        return;
    }
    while (ise(WHITE_SPACE()))
    {
        next();
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error");
        }
        if (ise(END_OF_INPUT()))
        {
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IDSZ2 ReadContext::readIDSZ() {
    char c[4];
    skipWhiteSpaces();
    // `'['`
    if (!ise(LEFT_SQUARE_BRACKET()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning IDSZ");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning IDSZ");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "unexpected character while scanning IDSZ");
        }
    }
    next();
    // `(<alphabetic>|<digit>|'_')^4`
    for (size_t i = 0; i < 4; ++i)
    {
        if (!ise(ALPHA()) && !ise(DIGIT()) && !ise(UNDERSCORE()))
        {
            if (ise(ERROR()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "read error while scanning IDSZ");
            }
            else if (ise(END_OF_INPUT()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "premature end of input while scanning IDSZ");
            }
            else
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "unexpected character while scanning IDSZ");
            }
        }
        c[i] = static_cast<char>(current());
        next();
    }
    // `']'`
    if (!ise(RIGHT_SQUARE_BRACKET()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning IDSZ");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning IDSZ");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "unexpected character while scanning IDSZ");
        }
    }
    next();
    return IDSZ2(c[0], c[1], c[2], c[3]);
}

//--------------------------------------------------------------------------------------------
bool ReadContext::skipToDelimiter(char delimiter, bool optional)
{
    while (true)
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error");
        }
        if (ise(END_OF_INPUT()))
        {
            if (optional)
            {
                return false;
            }
            else
            {
                throw Ego::Script::MissingDelimiterError(__FILE__, __LINE__, get_location(), delimiter);
            }
        }
        bool isDelimiter = is(delimiter);
        if (ise(NEW_LINE()))
        {
            new_line(nullptr);
        }
        else
        {
            next();
        }
        if (isDelimiter)
        {
            return true;
        }
    }
}

//--------------------------------------------------------------------------------------------

bool ReadContext::skipToColon(bool optional)
{
    return skipToDelimiter(':', optional);
}

//--------------------------------------------------------------------------------------------
std::string vfs_read_string_lit(ReadContext& ctxt)
{
    std::string temporary = ctxt.readStringLiteral();
    return str_decode(temporary);
}
//--------------------------------------------------------------------------------------------
std::string vfs_read_name(ReadContext& ctxt)
{
    return ctxt.readName();
}
//--------------------------------------------------------------------------------------------
void vfs_put_int( vfs_FILE* filewrite, const char* text, int ival )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %d\n", text, ival );
}

void vfs_put_int32(vfs_FILE *filewrite, const char *text, int32_t ival)
{
    vfs_printf(filewrite, "%s %d\n", text, ival);
}

//--------------------------------------------------------------------------------------------
void vfs_put_float( vfs_FILE* filewrite, const char* text, float fval )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %f\n", text, fval );
}

//--------------------------------------------------------------------------------------------
void vfs_put_ufp8( vfs_FILE* filewrite, const char* text, UFP8_T ival )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %f\n", text, FP8_TO_FLOAT( ival ) );
}

//--------------------------------------------------------------------------------------------
void vfs_put_sfp8( vfs_FILE* filewrite, const char* text, SFP8_T ival )
{
    /// @author ZZ
    /// @details This function kinda mimics fprintf for integers

    vfs_printf( filewrite, "%s %f\n", text, FP8_TO_FLOAT( ival ) );
}

//--------------------------------------------------------------------------------------------
void vfs_put_bool(vfs_FILE* filewrite, const char* text, bool truth)
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    true false statements

    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, truth ? "TRUE" : "FALSE" );
    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_damage_type( vfs_FILE* filewrite, const char* text, uint8_t damagetype )
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

    vfs_printf( filewrite, "%s", text );

    switch ( damagetype )
    {
        case DAMAGE_SLASH: vfs_printf( filewrite, "SLASH" ); break;
        case DAMAGE_CRUSH: vfs_printf( filewrite, "CRUSH" ); break;
        case DAMAGE_POKE : vfs_printf( filewrite, "POKE" ); break;
        case DAMAGE_HOLY : vfs_printf( filewrite, "HOLY" ); break;
        case DAMAGE_EVIL : vfs_printf( filewrite, "EVIL" ); break;
        case DAMAGE_FIRE : vfs_printf( filewrite, "FIRE" ); break;
        case DAMAGE_ICE  : vfs_printf( filewrite, "ICE" ); break;
        case DAMAGE_ZAP  : vfs_printf( filewrite, "ZAP" ); break;

        default:
        case DAMAGE_DIRECT : vfs_printf( filewrite, "DIRECT" ); break;
    }

    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_action( vfs_FILE* filewrite, const char* text, uint8_t action )
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

    vfs_printf( filewrite, "%s", text );

    switch ( action )
    {
        case ACTION_DA: vfs_printf( filewrite, "DANCE\n" );    break;
        case ACTION_UA: vfs_printf( filewrite, "UNARMED\n" ); break;
        case ACTION_TA: vfs_printf( filewrite, "THRUST\n" );  break;
        case ACTION_CA: vfs_printf( filewrite, "CHOP\n" );    break;
        case ACTION_SA: vfs_printf( filewrite, "SLASH\n" );   break;
        case ACTION_BA: vfs_printf( filewrite, "BASH\n" );    break;
        case ACTION_LA: vfs_printf( filewrite, "LONGBOW\n" ); break;
        case ACTION_XA: vfs_printf( filewrite, "XBOW\n" );    break;
        case ACTION_FA: vfs_printf( filewrite, "FLING\n" );   break;
        case ACTION_PA: vfs_printf( filewrite, "PARRY\n" );   break;
        case ACTION_ZA: vfs_printf( filewrite, "ZAP\n" );     break;
        case ACTION_WA: vfs_printf( filewrite, "WALK\n" );    break;
        case ACTION_HA: vfs_printf( filewrite, "HIT\n" );     break;
        case ACTION_KA: vfs_printf( filewrite, "KILLED\n" );  break;
        default:        vfs_printf( filewrite, "NONE\n" );    break;
    }
}

//--------------------------------------------------------------------------------------------
void vfs_put_gender_profile( vfs_FILE* filewrite, const char* text, GenderProfile gender )
{
    /// @author ZZ
    /// @details This function kinda mimics vfs_printf for the output of
    ///    MALE FEMALE OTHER statements

    vfs_printf( filewrite, "%s", text );

    switch ( gender )
    {
        case GenderProfile::Random: vfs_printf(filewrite, "Random\n"); break;
        case GenderProfile::Male: vfs_printf( filewrite, "Male\n" ); break;
        case GenderProfile::Female: vfs_printf( filewrite, "Female\n" ); break;
        case GenderProfile::Neuter: vfs_printf( filewrite, "Neuter\n" ); break;
    }
}

//--------------------------------------------------------------------------------------------
void vfs_put_range_raw(vfs_FILE* filewrite, idlib::interval<float> range) {
    float lowerbound = range.lower(),
        upperbound = range.upper();
    if (lowerbound == upperbound) {
        if (lowerbound == std::floor(lowerbound)) {
            vfs_printf(filewrite, "%d", (int)lowerbound);
        } else {
            vfs_printf(filewrite, "%4.2f", lowerbound);
        }
    } else {
        if (lowerbound != std::floor(lowerbound) || upperbound != std::floor(upperbound)) {
            vfs_printf(filewrite, "%4.2f-%4.2f", lowerbound, upperbound);
        } else {
            vfs_printf(filewrite, "%d-%d", (int)lowerbound, (int)upperbound);
        }
    }
}

void vfs_put_local_particle_profile_ref(vfs_FILE *filewrite, const char *text, const LocalParticleProfileRef& lppref)
{
    vfs_put_int(filewrite, text, lppref.get());
}

//--------------------------------------------------------------------------------------------
void vfs_put_range( vfs_FILE* filewrite, const char* text, idlib::interval<float> val )
{
    /// @author ZZ
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs. Try to print out the least amount of text.

    vfs_printf( filewrite, "%s", text );

    vfs_put_range_raw( filewrite, val );

    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_pair( vfs_FILE* filewrite, const char* text, IPair val )
{
    /// @author ZZ
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs

    idlib::interval<float> loc_range = pair_to_range(val);
    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, "%4.2f-%4.2f\n", loc_range.lower(), loc_range.upper() );
}

//--------------------------------------------------------------------------------------------
void vfs_put_string_under( vfs_FILE* filewrite, const char* text, const char* usename )
{
    /// @author ZZ
    /// @details This function mimics vfs_printf in spitting out
    ///    a name with underscore spaces

    char cTmp;
    int cnt;

    vfs_printf( filewrite, "%s", text );
    cnt = 0;
    cTmp = usename[0];
    cnt++;
    while ( CSTR_END != cTmp )
    {
        if ( ' ' == cTmp )
        {
            vfs_printf( filewrite, "_" );
        }
        else
        {
            vfs_printf( filewrite, "%c", cTmp );
        }

        cTmp = usename[cnt];
        cnt++;
    }

    vfs_printf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void vfs_put_idsz( vfs_FILE* filewrite, const char* text, const IDSZ2& idsz )
{
    vfs_printf( filewrite, "%s", text );
    vfs_printf(filewrite, "[%s]\n", idsz.toString().c_str());
}

//--------------------------------------------------------------------------------------------
void vfs_put_expansion(vfs_FILE *filewrite, const char *text, const IDSZ2& idsz, const LocalParticleProfileRef& lppref)
{
    vfs_printf(filewrite, "%s: [%s] %d\n", text, idsz.toString().c_str(), lppref.get());
}

void vfs_put_expansion(vfs_FILE *filewrite, const char *text, const IDSZ2& idsz, int value)
{
    vfs_printf(filewrite, "%s: [%s] %d\n", text, idsz.toString().c_str(), value);
}

//--------------------------------------------------------------------------------------------
void vfs_put_expansion_float( vfs_FILE* filewrite, const char* text, const IDSZ2& idsz, float value )
{
    /// @author ZF
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs for floating point values

    vfs_printf( filewrite, "%s: [%s] %4.2f\n", text, idsz.toString().c_str(), value );
}

//--------------------------------------------------------------------------------------------
void vfs_put_expansion_string( vfs_FILE* filewrite, const char* text, const IDSZ2& idsz, const char * str )
{
    /// @author ZF
    /// @details This function mimics vfs_printf in spitting out
    ///    damage/stat pairs for floating point values

    if ( !VALID_CSTR( str ) )
    {
        vfs_put_expansion( filewrite, text, idsz, 0 );
    }
    else
    {
        vfs_printf( filewrite, "%s: [%s] %s\n", text, idsz.toString().c_str(), str );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

idlib::interval<float> vfs_get_range(ReadContext& ctxt)
{
    // Read minimum.
    ctxt.skipWhiteSpaces();
    float from = ctxt.readRealLiteral();
    float to = from;
    // Read hyphen and maximum if present.
    ctxt.skipWhiteSpaces();
    if (ctxt.ise(ctxt.MINUS()))
    {
        ctxt.next();

        // Read maximum.
        ctxt.skipWhiteSpaces();
        to = ctxt.readRealLiteral();
    }

    return idlib::interval<float>(std::min(from, to), std::max(from, to));
}

//--------------------------------------------------------------------------------------------
idlib::interval<float> vfs_get_next_range(ReadContext& ctxt)
{
    /// @author ZZ
    /// @details This function reads a damage/stat range ( eg. 5-9 )

    ctxt.skipToColon(false);

    return vfs_get_range(ctxt);
}

//--------------------------------------------------------------------------------------------
/// Read a version number.
/// @code
/// versionNumber = '$VERSION_NUMBER' natural
/// @endcode
int vfs_get_version(ReadContext& ctxt)
{
    ctxt.skipWhiteSpaces();
    if (!ctxt.ise(ctxt.DOLLAR()))
    {
        return -1;
    }
    ctxt.next();
    std::string tag = ctxt.readName();
    if (tag != "FILE_VERSION")
    {
        return -1;
    }
    
    // Get the version number.
    return ctxt.readIntegerLiteral();
    
}

//--------------------------------------------------------------------------------------------
bool vfs_put_version( vfs_FILE* filewrite, const int version )
{
    if ( vfs_error( filewrite ) ) return false;

    return 0 != vfs_printf( filewrite, "$FILE_VERSION %i\n\n", version );
}

//--------------------------------------------------------------------------------------------
char vfs_get_next_printable(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readPrintable();
}

//--------------------------------------------------------------------------------------------

std::string ReadContext::readToEndOfLine()
{
    skipWhiteSpaces();
    clear_lexeme_text();
    while (!ise(END_OF_INPUT()))
    {
        if (ise(NEW_LINE()))
        {
            new_line(nullptr);
            break;
        }
        save_and_next();
    }
    return get_lexeme_text();
}

std::string ReadContext::readSingleLineComment()
{
    skipWhiteSpaces();
    clear_lexeme_text();
    if (!ise(SLASH()))
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                            "unexpected character while scanning single line comment");
    }
    next();
    if (!ise(SLASH()))
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                            "unexpected character while scanning single line comment");
    }
    next();
    skipWhiteSpaces();
    while (!ise(END_OF_INPUT()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning single line comment");
        }
        if (ise(NEW_LINE()))
        {
            new_line(nullptr);
            break;
        }
        save_and_next();
    }
    return get_lexeme_text();
}

char ReadContext::readPrintable()
{
    skipWhiteSpaces();
    if (ise(END_OF_INPUT()) || ise(ERROR()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning printable character");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning printable character");
        }
    }
    if (!ise(ALPHA()) && !ise(DIGIT()) && !ise(EXCLAMATION_MARK()) && !ise(QUESTION_MARK()) && !ise(EQUAL()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning printable character");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning a printable character");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "unexpected character while scanning a printable characters");
        }
    }
    char tmp = static_cast<char>(current());
    next();
    return tmp;
}

Ego::Script::DDLToken ReadContext::parseStringLiteral()
{
    idlib::hll::location startLocation = get_location();
    clear_lexeme_text();
    while (true)
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error");
        }
        else if (ise(TILDE()))
        {
            write_and_next('\t');
        }
        else if (ise(UNDERSCORE()))
        {
            write_and_next(' ');
        }
        else if (ise(NEW_LINE()) || ise(WHITE_SPACE()) || ise(END_OF_INPUT()))
        {
            break;
        }
        else
        {
            save_and_next();
        }
    }
    return Ego::Script::DDLToken(Ego::Script::DDLTokenKind::String, startLocation, get_lexeme_text());
}

Ego::Script::DDLToken ReadContext::parseCharacterLiteral() {
    idlib::hll::location startLocation = get_location();
    clear_lexeme_text();
    if (ise(END_OF_INPUT()) || ise(ERROR()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning character literal");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning character literal");
        }
    }
    if (!ise(SINGLE_QUOTE()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning character literal");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning character literal");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "unexpected character while scanning character literal");
        }
    }
    next();
    if (ise(BACKSLASH()))
    {
        next();
        if (ise(SINGLE_QUOTE()))
        {
            write_and_next('\'');
        }
        else if (is('n'))
        {
            write_and_next('\n');
        }
        else if (is('t'))
        {
            write_and_next('\t');
        }
        else if (ise(BACKSLASH()))
        {
            write_and_next('\\');
        }
        else
        {
            if (ise(ERROR()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "read error while scanning character literal");
            }
            else if (ise(END_OF_INPUT()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "premature end of input while scanning character literal");
            }
            else
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "unknown/unsupported escape sequence");
            }
        }
    }
    else
    {
        if (ise(END_OF_INPUT()) || ise(ERROR()))
        {
            if (ise(ERROR()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "read error while scanning character literal");
            }
            else if (ise(END_OF_INPUT()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "empty character literal");
            }
        }
        save_and_next();
    }
    if (!ise(SINGLE_QUOTE())) {
        throw Ego::Script::MissingDelimiterError(__FILE__, __LINE__, get_location(), '\'');
    }
    next();
    return Ego::Script::DDLToken(Ego::Script::DDLTokenKind::Character, startLocation, get_lexeme_text());
}

Ego::Script::DDLToken ReadContext::parseIntegerLiteral()
{
    idlib::hll::location startLocation = get_location();
    clear_lexeme_text();
    if (ise(PLUS()) || ise(MINUS()))
    {
        save_and_next();
    }
    if (!ise(DIGIT()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning integer literal");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning integer literal");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "unexpected character while scanning integer literal");
        }
    }
    do
    {
        save_and_next();
    } while (ise(DIGIT()));
    if (is('e') || is('E'))
    {
        save_and_next();
        if (ise(PLUS()))
        {
            save_and_next();
        }
        if (!ise(DIGIT()))
        {
            if (ise(ERROR()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "read error while scanning integer literal");
            }
            else if (ise(END_OF_INPUT()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "premature end of input while scanning integer literal");
            }
            else
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "unexpected character while scanning integer literal");
            }
        }
        do
        {
            save_and_next();
        } while (ise(DIGIT()));
    }
    return Ego::Script::DDLToken(Ego::Script::DDLTokenKind::Integer, startLocation, get_lexeme_text());
}

Ego::Script::DDLToken ReadContext::parseNaturalLiteral()
{
    clear_lexeme_text();
    idlib::hll::location startLocation = get_location();
    if (ise(PLUS()))
    {
        save_and_next();
    }
    if (!ise(DIGIT()))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning natural literal");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning natural literal");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "unexpected character while scanning natural literal");
        }
    }
    do
    {
        save_and_next();
    } while (ise(DIGIT()));
    if (is('e') || is('E'))
    {
        save_and_next();
        if (ise(PLUS()))
        {
            save_and_next();
        }
        if (!ise(DIGIT()))
        {
            if (ise(ERROR()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "read error while scanning natural literal");
            }
            else if (ise(END_OF_INPUT()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "premature end of input while scanning natural literal");
            }
            else
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "unexpected character while scanning natural literal");
            }
        }
        do
        {
            save_and_next();
        } while (ise(DIGIT()));
    }
    return Ego::Script::DDLToken(Ego::Script::DDLTokenKind::Integer, startLocation, get_lexeme_text());
}

Ego::Script::DDLToken ReadContext::parseRealLiteral()
{
    clear_lexeme_text();
    idlib::hll::location startLocation = get_location();
    if (is('+') || is('-'))
    {
        save_and_next();
    }
    if (is('.'))
    {
        save_and_next();
        if (!ise(DIGIT()))
        {
            if (ise(ERROR()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "read error while scanning real literal");
            }
            else if (ise(END_OF_INPUT()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "premature end of input while scanning real literal");
            }
            else
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "unexpected character while scanning real literal");
            }
        }
        do
        {
            save_and_next();
        } while (ise(DIGIT()));
    }
    else if (ise(DIGIT()))
    {
        do
        {
            save_and_next();
        } while (ise(DIGIT()));
        if (is('.'))
        {
            save_and_next();
            while (ise(DIGIT()))
            {
                save_and_next();
            }
        }
    }
    if (is('e') || is('E'))
    {
        save_and_next();
        if (is('+') || is('-'))
        {
            save_and_next();
        }
        if (!ise(DIGIT()))
        {
            if (ise(ERROR()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "read error while scanning real literal exponent");
            }
            else if (ise(END_OF_INPUT()))
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "premature end of input while scanning real literal exponent");
            }
            else
            {
                throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                    "unexpected character while scanning real literal exponent");
            }
        }
        do
        {
            save_and_next();
        } while (ise(DIGIT()));
    }
    return Ego::Script::DDLToken(Ego::Script::DDLTokenKind::Real, startLocation, get_lexeme_text());
}

std::string ReadContext::readStringLiteral() {
    skipWhiteSpaces();
    auto token = parseStringLiteral();
    return Ego::Script::DDLTokenDecoder<std::string>()(token);
}

char ReadContext::readCharacterLiteral() {
    skipWhiteSpaces();
    auto token = parseCharacterLiteral();
    return Ego::Script::DDLTokenDecoder<char>()(token);
}

signed int ReadContext::readIntegerLiteral() {
    skipWhiteSpaces();
    auto token = parseIntegerLiteral();
    return Ego::Script::DDLTokenDecoder<signed int>()(token);
}

unsigned int ReadContext::readNaturalLiteral() {
    skipWhiteSpaces();
    auto token = parseNaturalLiteral();
    return Ego::Script::DDLTokenDecoder<unsigned int>()(token);
}

float ReadContext::readRealLiteral() {
    skipWhiteSpaces();
    auto token = parseRealLiteral();
    return Ego::Script::DDLTokenDecoder<float>()(token);
}

//--------------------------------------------------------------------------------------------
UFP8_T vfs_get_ufp8(ReadContext& ctxt)
{
    float x = ctxt.readRealLiteral();
    if (x < 0.0f)
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, ctxt.get_location(),
                                            "unable to convert current lexeme to a unsigned fixed-point number");
    }
    return FLOAT_TO_FP8(x);
}

//--------------------------------------------------------------------------------------------
SFP8_T vfs_get_sfp8(ReadContext& ctxt)
{
    float x = ctxt.readRealLiteral();
    return FLOAT_TO_FP8(x);
}

//--------------------------------------------------------------------------------------------
int vfs_get_next_int(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readIntegerLiteral();
}

int32_t vfs_get_next_int32(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    ctxt.skipWhiteSpaces();
    auto token = ctxt.parseIntegerLiteral();
    return Ego::Script::DDLTokenDecoder<int32_t>()(token);
}

unsigned int vfs_get_next_nat(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readNaturalLiteral();
}

UFP8_T vfs_get_next_ufp8(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_get_ufp8(ctxt);
}

SFP8_T vfs_get_next_sfp8(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_get_sfp8(ctxt);
}

LocalParticleProfileRef vfs_get_next_local_particle_profile_ref(ReadContext& ctxt) {
    ctxt.skipToColon(false);
    return vfs_get_local_particle_profile_ref(ctxt);
}

//--------------------------------------------------------------------------------------------
void vfs_read_string(ReadContext& ctxt, char *str, size_t max)
{
    ctxt.skipWhiteSpaces();
    ctxt.clear_lexeme_text();
    if (!max)
    {
        str[0] = '\0';
        return;
    }
    else
    {
        size_t cur = 0;
        while (cur < max && !ctxt.ise(ctxt.NEW_LINE()) && !ctxt.ise(ctxt.WHITE_SPACE()) &&
               !ctxt.ise(ctxt.END_OF_INPUT()) && !ctxt.ise(ctxt.ERROR()))
        {
            ctxt.save_and_next();
            cur++;
        }
        if (ctxt.ise(ctxt.ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, ctxt.get_location(),
                                                "read error while reading string literal");
        }
        IDLIB_DEBUG_ASSERT(ctxt.get_lexeme_text().size() == cur && cur <= max);
        strcpy(str, ctxt.get_lexeme_text().c_str());
        return;
    }
}

std::string vfs_get_next_string_lit(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_read_string_lit(ctxt);
}

//--------------------------------------------------------------------------------------------
float vfs_get_next_float(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readRealLiteral();
}

//--------------------------------------------------------------------------------------------

std::string vfs_get_next_name(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_read_name(ctxt);
}

//--------------------------------------------------------------------------------------------
IDSZ2 vfs_get_next_idsz(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readIDSZ();
}

//--------------------------------------------------------------------------------------------

LocalParticleProfileRef vfs_get_local_particle_profile_ref(ReadContext& ctxt) {
    return LocalParticleProfileRef(ctxt.readIntegerLiteral());
}

DamageType vfs_get_damage_type(ReadContext& ctxt)
{
    static const Ego::Script::EnumDescriptor<DamageType> enumDescriptor
    (
        "DamageType",
        {
            // Slash.
            { "Slash", DAMAGE_SLASH },
            { "SLASH", DAMAGE_SLASH },
            { "S",     DAMAGE_SLASH },
            // Crush.
            { "Crush", DAMAGE_CRUSH },
            { "CRUSH", DAMAGE_CRUSH },
            { "C",     DAMAGE_CRUSH },
            // Poke.
            { "Poke",  DAMAGE_POKE  },
            { "POKE",  DAMAGE_POKE  },
            { "P",     DAMAGE_POKE  },
            // Holy.
            { "Holy",  DAMAGE_HOLY  },
            { "HOLY",  DAMAGE_HOLY  },
            { "H",     DAMAGE_HOLY  },
            // Evil.
            { "Evil",  DAMAGE_EVIL  },
            { "EVIL",  DAMAGE_EVIL  },
            { "E",     DAMAGE_EVIL  },
            // Fire.
            { "Fire",  DAMAGE_FIRE  },
            { "FIRE",  DAMAGE_FIRE  },
            { "F",     DAMAGE_FIRE  },
            // Ice.
            { "Ice",   DAMAGE_ICE   },
            { "ICE",   DAMAGE_ICE   },
            { "I",     DAMAGE_ICE   },
            // Zap.
            { "Zap",   DAMAGE_ZAP   },
            { "ZAP",   DAMAGE_ZAP   },
            { "Z",     DAMAGE_ZAP   },
            // Direct.
            { "Direct",DAMAGE_DIRECT  },
            { "DIRECT",DAMAGE_DIRECT  },
            { "D",     DAMAGE_DIRECT  },

            // Direct used to be called 'NONE' before. Kept here for backwards compatability
            { "None",  DAMAGE_DIRECT  },
            { "NONE",  DAMAGE_DIRECT  },
            { "N",     DAMAGE_DIRECT  },
        }
    );
    return ctxt.readEnum(enumDescriptor, DAMAGE_DIRECT);
}

//--------------------------------------------------------------------------------------------
DamageType vfs_get_next_damage_type(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return vfs_get_damage_type(ctxt);
}

//--------------------------------------------------------------------------------------------
void ReadContext::readName0()
{
    if (!ise(ALPHA()) && !ise(UNDERSCORE()))
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                            "invalid name");
    }
    do
    {
        save_and_next();
    } while (ise(ALPHA()) || ise(DIGIT()) || ise(UNDERSCORE()));
}

void ReadContext::readOldString0()
{
    static const auto p = idlib::parsing_expression::ordered_choice(WHITE_SPACE(), NEW_LINE(), END_OF_INPUT());
    if (ise(p))
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                            "invalid old string");
    }
    do
    {
        save_and_next();
    } while (!ise(p));
}

std::string ReadContext::readOldString()
{
    skipWhiteSpaces();
    clear_lexeme_text();
    readOldString0();
    return get_lexeme_text();
}

std::string ReadContext::readName()
{
    skipWhiteSpaces();
    clear_lexeme_text();
    readName0();
    return get_lexeme_text();
}

void ReadContext::readReference0()
{
    if (!is('%'))
    {
        if (ise(ERROR()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "read error while scanning reference literal");
        }
        else if (ise(END_OF_INPUT()))
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "premature end of input while scanning reference literal");
        }
        else
        {
            throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                                "unexpected character while scanning reference literal");
        }
    }
    save_and_next();
    readName0();
}

std::string ReadContext::readReference()
{
    skipWhiteSpaces();
    clear_lexeme_text();
    readReference0();
    return get_lexeme_text();
}

bool ReadContext::readBool()
{
    std::string name = readName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "true" || name == "t")
    {
        return true;
    }
    else if (name == "false" || name == "f")
    {
        return false;
    }
    else
    {
        throw idlib::hll::compilation_error(__FILE__, __LINE__, idlib::hll::compilation_error_kind::lexical, get_location(),
                                            "unexpected character while scanning boolean literal");
    }
}

//--------------------------------------------------------------------------------------------

bool vfs_get_next_bool(ReadContext& ctxt) {
    ctxt.skipToColon(false);
    ctxt.skipWhiteSpaces();
    return ctxt.readBool();
}

//--------------------------------------------------------------------------------------------
bool ego_texture_exists_vfs(const std::string &filename)
{
    // Try all different formats.
    for (const auto& loader : Ego::ImageManager::get())
    {
        for (const auto& extension : loader.getExtensions()) {
            // Build the full file name.
            std::string fullFilename = filename + extension;
            if (vfs_exists(fullFilename)) {
                return true;
            }
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------
DamageModifier vfs_get_damage_modifier(ReadContext& ctxt)
{
    static const Ego::Script::EnumDescriptor<DamageModifier> enumDescriptor
        (
        "damageModifier",
        {
            { "T", DamageModifier::DAMAGEINVERT },
            { "C", DamageModifier::DAMAGECHARGE },
            { "M", DamageModifier::DAMAGEMANA },
            { "I", DamageModifier::DAMAGEINVICTUS },
            { "F", DamageModifier::NONE },
        }
    );
    return ctxt.readEnum(enumDescriptor, DamageModifier::NONE);
}

//--------------------------------------------------------------------------------------------
float vfs_get_damage_resist(ReadContext& ctxt)
{
    /// @todo Ugly hack to allow it to work with the old damage system assume that numbers below 4 are shifts.
    float resistance = ctxt.readRealLiteral();

    //@note ZF> uncomment to get damage reduction roughly same as old system
    //if (resistance == 1) resistance = 16.7f;         //~50% reduction, about same as shift 1
    //else if (resistance == 2) resistance = 50.0f;    //75% reduction, same as shift 2
    //else if (resistance == 3) resistance = 150.0f;   //90% reduction, same as shift 3
    
    switch(static_cast<int>(resistance))
    {
        case 0: resistance = -11.203f; break;    //-100% damage reduction
        case 1: resistance = 0.0f;     break;    //0% damage reduction
        case 2: resistance = 16.7f;    break;    //50% damage reduction
        case 3: resistance = 50.0f;    break;    //75% damage reduction
    }

    return resistance;
}

#pragma pop_macro("ERROR")
