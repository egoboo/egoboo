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

#include "egolib/Log/_Include.hpp"
#include "egolib/strutil.h"
#include "egolib/platform.h"
#include "egolib/egoboo_setup.h"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/_math.h"
// includes for egoboo constants
#include "egolib/Graphics/ModelDescriptor.hpp"                    // for ACTION_* constants

ReadContext::ReadContext(const std::string& fileName) :
    AbstractReader(fileName, 5012)
{
}

ReadContext::~ReadContext()
{
}

float ReadContext::toReal() const
{
    float temporary;
    auto lexeme = _buffer.toString();
    if (!Decoder<float>()(lexeme,temporary))
    {
        throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                        "unable to convert current lexeme `" + lexeme + "` into a value of type "
                                        "`float`");
    }
    return temporary;
}

void ReadContext::skipWhiteSpaces()
{
    if (is(Traits::startOfInput()))
    {
        next();
    }
    if (is(Traits::error()))
    {
        throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                        "read error");
    }
    if (is(Traits::endOfInput()))
    {
        return;
    }
    while (isWhiteSpace())
    {
        next();
        if (is(Traits::error()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "read error");
        }
        if (is(Traits::endOfInput()))
        {
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IDSZ2 ReadContext::readIDSZ() {
	char c[4];
	if (is(Traits::startOfInput()))
	{
		next();
	}
	skipWhiteSpaces();
	// `'['`
	if (!is('['))
	{
		if (is(Traits::error()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"read error while scanning IDSZ");
		}
		else if (is(Traits::endOfInput()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"premature end of input while scanning IDSZ");
		}
		else
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"unexpected character while scanning IDSZ");
		}
	}
	next();
	// `(<alphabetic>|<digit>|'_')^4`
	for (size_t i = 0; i < 4; ++i)
	{
		if (!isAlpha() && !isDigit() && !is('_'))
		{
			if (is(Traits::error()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"read error while scanning IDSZ");
			}
			else if (is(Traits::endOfInput()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"premature end of input while scanning IDSZ");
			}
			else
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"unexpected character while scanning IDSZ");
			}
		}
		c[i] = static_cast<char>(current());
		next();
	}
	// `']'`
	if (!is(']'))
	{
		if (is(Traits::error()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"read error while scanning IDSZ");
		}
		else if (is(Traits::endOfInput()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"premature end of input while scanning IDSZ");
		}
		else
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"unexpected character while scanning IDSZ");
		}
	}
	next();
	return IDSZ2(c[0], c[1], c[2], c[3]);
}

//--------------------------------------------------------------------------------------------
bool ReadContext::skipToDelimiter(char delimiter, bool optional)
{
    if (!Traits::isValid(delimiter))
    {
        std::invalid_argument("!Ego::VFS::Traits<char>::isValid(delimiter)");
    }
    if (is(Traits::startOfInput()))
    {
        next();
    }
    while (true)
    {
        if (is(Traits::error()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "read error");
        }
        if (is(Traits::endOfInput()))
        {
            if (optional)
            {
                return false;
            }
            else
            {
                throw MissingDelimiterError(__FILE__, __LINE__, Location(getFileName(), getLineNumber()), delimiter);
            }
        }
        bool isDelimiter = is(delimiter);
        if (isNewLine())
        {
            skipNewLine();
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
void vfs_read_string_lit(ReadContext& ctxt, std::string& literal)
{
    std::string temporary = ctxt.readStringLiteral();
    temporary = str_decode(temporary);
    literal = temporary;
}
//--------------------------------------------------------------------------------------------
void vfs_read_name(ReadContext& ctxt, std::string& buffer)
{
    buffer = ctxt.readName();
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
void vfs_put_damage_type( vfs_FILE* filewrite, const char* text, Uint8 damagetype )
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
void vfs_put_action( vfs_FILE* filewrite, const char* text, Uint8 action )
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
void vfs_put_range_raw(vfs_FILE* filewrite, Ego::Math::Interval<float> range) {
    float lowerbound = range.getLowerbound(),
        upperbound = range.getUpperbound();
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
void vfs_put_range( vfs_FILE* filewrite, const char* text, Ego::Math::Interval<float> val )
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

    Ego::Math::Interval<float> loc_range = pair_to_range(val);
    vfs_printf( filewrite, "%s", text );
    vfs_printf( filewrite, "%4.2f-%4.2f\n", loc_range.getLowerbound(), loc_range.getUpperbound() );
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

Ego::Math::Interval<float> vfs_get_range(ReadContext& ctxt)
{
    // Read minimum.
    ctxt.skipWhiteSpaces();
    float from = ctxt.readRealLiteral();
    float to = from;
    // Read hyphen and maximum if present.
    ctxt.skipWhiteSpaces();
    if (ctxt.is('-'))
    {
        ctxt.next();

        // Read maximum.
        ctxt.skipWhiteSpaces();
        to = ctxt.readRealLiteral();
    }

    return Ego::Math::Interval<float>(std::min(from, to), std::max(from, to));
}

//--------------------------------------------------------------------------------------------
Ego::Math::Interval<float> vfs_get_next_range(ReadContext& ctxt)
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
    if (ctxt.is(ReadContext::Traits::startOfInput()))
    {
        ctxt.next();
    }
    ctxt.skipWhiteSpaces();
    if (!ctxt.is('$'))
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
    if (is(Traits::startOfInput()))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    while (!is(Traits::endOfInput()))
    {
        if (isNewLine())
        {
            skipNewLine();
            break;
        }
        saveAndNext();
    }
    return _buffer.toString();
}

std::string ReadContext::readSingleLineComment()
{
    if (is(Traits::startOfInput()))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    if (!is('/'))
    {
        throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                        "unexpected character while scanning single line comment");
    }
    next();
    if (!is('/'))
    {
        throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                        "unexpected character while scanning single line comment");
    }
    next();
    skipWhiteSpaces();
    while (!is(Traits::endOfInput()))
    {
        if (is(Traits::error()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "read error while scanning single line comment");
        }
        if (isNewLine())
        {
            skipNewLine();
            break;
        }
        saveAndNext();
    }
    return _buffer.toString();
}

char ReadContext::readPrintable()
{
    if (is(Traits::startOfInput()))
    {
        next();
    }
    skipWhiteSpaces();
    if (is(Traits::endOfInput()) || is(Traits::error()))
    {
        if (is(Traits::error()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "read error while scanning printable character");
        }
        else
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "premature end of input while scanning printable character");
        }
    }
    if (!isAlpha() && !isDigit() && !is('!') && !is('?') && !is('='))
    {
        if (is(Traits::error()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "read error while scanning printable character");
        }
        else if (is(Traits::endOfInput()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "premature end of input while scanning a printable character");
        }
        else
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "unexpected character while scanning a printable characters");
        }
    }
    char tmp = static_cast<char>(current());
    next();
    return tmp;
}

DDLToken ReadContext::parseStringLiteral()
{
	Location startLocation(getFileName(), getLineNumber());
	_buffer.clear();
	if (is(Traits::startOfInput()))
	{
		next();
	}
	_buffer.clear();
	while (true)
	{
		if (is(Traits::error()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"read error");
		}
		else if (is('~'))
		{
			writeAndNext('\t');
		}
		else if (is('_'))
		{
			writeAndNext(' ');
		}
		else if (isNewLine() || isWhiteSpace() || is(Traits::endOfInput()))
		{
			break;
		}
		else
		{
			saveAndNext();
		}
	}
	return DDLToken(DDLTokenKind::String, startLocation, _buffer.toString());
}

DDLToken ReadContext::parseCharacterLiteral() {
	Location startLocation(getFileName(), getLineNumber());
	_buffer.clear();
	if (is(Traits::startOfInput()))
	{
		next();
	}
	if (is(Traits::endOfInput()) || is(Traits::error()))
	{
		if (is(Traits::error()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"read error while scanning character literal");
		}
		else if (is(Traits::endOfInput()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"premature end of input while scanning character literal");
		}
	}
	if (!is('\''))
	{
		if (is(Traits::error()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"read error while scanning character literal");
		}
		else if (is(Traits::endOfInput()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"premature end of input while scanning character literal");
		}
		else
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"unexpected character while scanning character literal");
		}
	}
	next();
	if (is('\\'))
	{
		next();
		if (is('\''))
		{
			writeAndNext('\'');
		}
		else if (is('n'))
		{
			writeAndNext('\n');
		}
		else if (is('t'))
		{
			writeAndNext('\t');
		}
		else if (is('\\'))
		{
			writeAndNext('\\');
		}
		else
		{
			if (is(Traits::error()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"read error while scanning character literal");
			}
			else if (is(Traits::endOfInput()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"premature end of input while scanning character literal");
			}
			else
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"unknown/unsupported escape sequence");
			}
		}
	}
	else
	{
		if (is(Traits::endOfInput()) || is(Traits::error()))
		{
			if (is(Traits::error()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"read error while scanning character literal");
			}
			else if (is(Traits::endOfInput()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"empty character literal");
			}
		}
		saveAndNext();
	}
	if (!is('\'')) {
		throw MissingDelimiterError(__FILE__, __LINE__, Location(getFileName(), getLineNumber()), '\'');
	}
	next();
	return DDLToken(DDLTokenKind::Character, startLocation, _buffer.toString());
}

DDLToken ReadContext::parseIntegerLiteral()
{
	Location startLocation(getFileName(), getLineNumber());
	_buffer.clear();
	if (is(Traits::startOfInput()))
	{
		next();
	}
	if (is('+') || is('-'))
	{
		saveAndNext();
	}
	if (!isDigit())
	{
		if (is(Traits::error()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"read error while scanning integer literal");
		}
		else if (is(Traits::endOfInput()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"premature end of input while scanning integer literal");
		}
		else
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"unexpected character while scanning integer literal");
		}
	}
	do
	{
		saveAndNext();
	} while (isDigit());
	if (is('e') || is('E'))
	{
		saveAndNext();
		if (is('+'))
		{
			saveAndNext();
		}
		if (!isDigit())
		{
			if (is(Traits::error()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"read error while scanning integer literal");
			}
			else if (is(Traits::endOfInput()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"premature end of input while scanning integer literal");
			}
			else
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"unexpected character while scanning integer literal");
			}
		}
		do
		{
			saveAndNext();
		} while (isDigit());
	}
    return DDLToken(DDLTokenKind::Integer, startLocation, _buffer.toString());
}

DDLToken ReadContext::parseNaturalLiteral()
{
	_buffer.clear();
	Location startLocation(getFileName(), getLineNumber());
	if (is(Traits::startOfInput()))
	{
		next();
	}
	if (is('+'))
	{
		saveAndNext();
	}
	if (!isDigit())
	{
		if (is(Traits::error()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"read error while scanning natural literal");
		}
		else if (is(Traits::endOfInput()))
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"premature end of input while scanning natural literal");
		}
		else
		{
			throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
				"unexpected character while scanning natural literal");
		}
	}
	do
	{
		saveAndNext();
	} while (isDigit());
	if (is('e') || is('E'))
	{
		saveAndNext();
		if (is('+'))
		{
			saveAndNext();
		}
		if (!isDigit())
		{
			if (is(Traits::error()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"read error while scanning natural literal");
			}
			else if (is(Traits::endOfInput()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"premature end of input while scanning natural literal");
			}
			else
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"unexpected character while scanning natural literal");
			}
		}
		do
		{
			saveAndNext();
		} while (isDigit());
	}
	return DDLToken(DDLTokenKind::Integer, startLocation, _buffer.toString());
}

DDLToken ReadContext::parseRealLiteral()
{
	_buffer.clear();
	Location startLocation(getFileName(), getLineNumber());
	if (is(Traits::startOfInput()))
	{
		next();
	}
	if (is('+') || is('-'))
	{
		saveAndNext();
	}
	if (is('.'))
	{
		saveAndNext();
		if (!isDigit())
		{
			if (is(Traits::error()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"read error while scanning real literal");
			}
			else if (is(Traits::endOfInput()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"premature end of input while scanning real literal");
			}
			else
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"unexpected character while scanning real literal");
			}
		}
		do
		{
			saveAndNext();
		} while (isDigit());
	}
	else if (isDigit())
	{
		do
		{
			saveAndNext();
		} while (isDigit());
		if (is('.'))
		{
			saveAndNext();
			while (isDigit())
			{
				saveAndNext();
			}
		}
	}
	if (is('e') || is('E'))
	{
		saveAndNext();
		if (is('+') || is('-'))
		{
			saveAndNext();
		}
		if (!isDigit())
		{
			if (is(Traits::error()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"read error while scanning real literal exponent");
			}
			else if (is(Traits::endOfInput()))
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"premature end of input while scanning real literal exponent");
			}
			else
			{
				throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
					"unexpected character while scanning real literal exponent");
			}
		}
		do
		{
			saveAndNext();
		} while (isDigit());
	}
	return DDLToken(DDLTokenKind::Real, startLocation, _buffer.toString());
}

std::string ReadContext::readStringLiteral() {
	skipWhiteSpaces();
	auto token = parseStringLiteral();
	return DDLTokenDecoder<std::string>()(token);
}

char ReadContext::readCharacterLiteral() {
	skipWhiteSpaces();
	auto token = parseCharacterLiteral();
	return DDLTokenDecoder<char>()(token);
}

signed int ReadContext::readIntegerLiteral() {
	skipWhiteSpaces();
	auto token = parseIntegerLiteral();
	return DDLTokenDecoder<signed int>()(token);
}

unsigned int ReadContext::readNaturalLiteral() {
    skipWhiteSpaces();
	auto token = parseNaturalLiteral();
	return DDLTokenDecoder<unsigned int>()(token);
}

float ReadContext::readRealLiteral() {
	skipWhiteSpaces();
	auto token = parseRealLiteral();
	return DDLTokenDecoder<float>()(token);
}

//--------------------------------------------------------------------------------------------
UFP8_T vfs_get_ufp8(ReadContext& ctxt)
{
    float x = ctxt.readRealLiteral();
    if (x < 0.0f)
    {
        throw LexicalErrorException(__FILE__, __LINE__, Location(ctxt.getFileName(),ctxt.getLineNumber()),
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
    return DDLTokenDecoder<int32_t>()(token);
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
    ctxt._buffer.clear();
    if (!max)
    {
        str[0] = '\0';
        return;
    }
    else
    {
        size_t cur = 0;
        while (cur < max && !ctxt.isNewLine() && !ctxt.isWhiteSpace() &&
               !ctxt.is(ReadContext::Traits::endOfInput()) && !ctxt.is(ReadContext::Traits::error()))
        {
            ctxt.saveAndNext();
            cur++;
        }
        if (ctxt.is(ReadContext::Traits::error()))
        {
            throw LexicalErrorException(__FILE__,__LINE__,Location(ctxt.getFileName(),ctxt.getLineNumber()),
                                            "read error while reading string literal");
        }
        EGOBOO_ASSERT(ctxt._buffer.getSize() == cur && cur <= max);
        strcpy(str, ctxt._buffer.toString().c_str());
        return;
    }
}

void vfs_get_next_string_lit(ReadContext& ctxt, std::string& str)
{
    ctxt.skipToColon(false);
    vfs_read_string_lit(ctxt, str);
}

//--------------------------------------------------------------------------------------------
float vfs_get_next_float(ReadContext& ctxt)
{
    ctxt.skipToColon(false);
    return ctxt.readRealLiteral();
}

//--------------------------------------------------------------------------------------------

void vfs_get_next_name(ReadContext& ctxt, std::string& buffer)
{
    ctxt.skipToColon(false);
    vfs_read_name(ctxt, buffer);
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
    static const EnumDescriptor<DamageType> enumDescriptor
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
    if (!isAlpha() && !is('_'))
    {
        throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                        "invalid name");
    }
    do
    {
        saveAndNext();
    } while (isAlpha() || isDigit() || is('_') || is('\''));
}

std::string ReadContext::readName()
{
    if (is(Traits::startOfInput()))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    readName0();
    return toString();
}

void ReadContext::readReference0()
{
    if (!is('%'))
    {
        if (is(Traits::error()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "read error while scanning reference literal");
        }
        else if (is(Traits::endOfInput()))
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "premature end of input while scanning reference literal");
        }
        else
        {
            throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
                                            "unexpected character while scanning reference literal");
        }
    }
    saveAndNext();
    readName0();
}

std::string ReadContext::readReference()
{
    if (is(Traits::startOfInput()))
    {
        next();
    }
    skipWhiteSpaces();
    _buffer.clear();
    readReference0();
    return toString();
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
        throw LexicalErrorException(__FILE__, __LINE__, Location(getFileName(), getLineNumber()),
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
    static const EnumDescriptor<DamageModifier> enumDescriptor
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
