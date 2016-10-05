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

/// @file egolib/strutil.c
/// @brief String manipulation functions.
/// @details

#include "egolib/strutil.h"
#include "egolib/fileutil.h" /**<< @tood Remove this include. */
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/platform.h" /**<< @todo Remove this include. */

//--------------------------------------------------------------------------------------------
std::string str_decode(const std::string& source) {
    static const auto transcode = [](char source) {
        switch (source) {
            case '_': return ' ';
            case '~': return '\t';
            default: return source;
        }
    };
    auto temporary = source;
    std::transform(temporary.begin(), temporary.end(), temporary.begin(), transcode);
    return temporary;
}

std::string str_encode(const std::string& source) {
    static const auto transcode = [](char source) {
        switch (source) {
            case ' ': return '_';
            case '\t': return '~';
            default: return source;
        };
    };
    auto temporary = source;
    std::transform(temporary.begin(), temporary.end(), temporary.begin(), transcode);
    return temporary;
}

//--------------------------------------------------------------------------------------------
std::string str_clean_path(const std::string& pathname) {
    struct predicate {
        bool operator()(char a, char b) const {
            return (a == '/' || a == '\\') && (b == '/'||  b == '\\');
        }
    };
    auto temporary = pathname;
    temporary.erase(std::unique(temporary.begin(), temporary.end(), predicate()), temporary.end());
    return temporary;
}

//--------------------------------------------------------------------------------------------
std::string str_convert_slash_net(const Ego::VfsPath& path) {
    return path.string(Ego::VfsPath::Kind::Network);
}

//--------------------------------------------------------------------------------------------
std::string str_convert_slash_sys(const std::string& pathname) {
    static const auto transcode = [](char source) {
        if (source == C_SLASH_CHR || source == C_BACKSLASH_CHR) return SLASH_CHR;
        else return source;
    };
    auto temporary = pathname;
    std::transform(temporary.begin(), temporary.end(), temporary.begin(), transcode);
    return str_clean_path(temporary);
}

//--------------------------------------------------------------------------------------------
std::string str_append_slash_net(const std::string& filename) {
	size_t l = filename.length();
	if (!l) {
		return NET_SLASH_STR;
	} else if (C_SLASH_CHR != filename[l - 1] && C_BACKSLASH_CHR != filename[l - 1]) {
		return filename + NET_SLASH_STR;
	} else {
		return filename;
	}
}

//--------------------------------------------------------------------------------------------
std::string str_append_slash( const std::string& filename ) {
    size_t l = filename.length();
	if (!l) {
		return SLASH_STR;
	} else if (C_SLASH_CHR != filename[l-1] && C_BACKSLASH_CHR != filename[l-1]) {
		return filename + SLASH_STR;
	} else {
		return filename;
	}
}

//--------------------------------------------------------------------------------------------

std::string str_encode_path( const std::string& objectName)
{
	std::string encodedName = objectName + ".obj";
	// alphabetic uppercase -> alphabetic lowercase
	// space -> underscore
	auto f = [](const char chr) -> char {
		if (std::isalpha(chr)) {
			return std::tolower(chr);
		} else if (std::isspace(chr)) {
			return '_';
		} else {
			return chr;
		}};
	transform(encodedName.begin(), encodedName.end(), encodedName.begin(), f);
	return encodedName;
}

//--------------------------------------------------------------------------------------------

std::string add_linebreak_cpp(const std::string& text, size_t lineLength) {
    if (0 == text.length() || 0 == lineLength) return text;

    struct line_t {
        line_t(size_t start, size_t end) : start(start), end(end) {}
        size_t length() const {
            return end - start;
        }
        size_t start, end;
    };
    std::string newText = text;
    line_t line(0, 0);
    // Memoize the last whitespace in this line.
    size_t last_ws = std::string::npos;
    while (newText[line.end] != '\0') {
        if (newText[line.end] == ' ') {
            // If a whitespace is encountered, memoize that whitespace.
            last_ws = line.end;
            // Expand line.
            line.end++;
        } else if (newText[line.end] == '\n') {
            // Begin a new line.
            line.start = line.end + 1;
            line.end = line.start;
            // No whitespace in that new line (so far).
            last_ws = std::string::npos;
        } else {
            // Expand line.
            line.end++;
        }
        // If the sub-length exceeds the limit
        if (line.length() > lineLength) {
            // If a whitespace exists in that line ...
            if (last_ws != std::string::npos) {
                // If a whitespace character in this line was found replace it by a newline character.
                // This effectively starts a new line.
                newText[last_ws] = '\n';
                line.start = last_ws + 1;
            } else {
                // Otherwise there is nothing we can do: advance to end of string, whitespace, or end of line.
                while (newText[line.end] != '\n' && newText[line.end] != ' ' && newText[line.end] != '\0') line.end++;
            }
        }
    }
    return newText;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// gcc does not define these functions on linux (at least not Ubuntu),
// but it is defined under MinGW, which is yucky.
// I actually had to spend like 45 minutes looking up the compiler flags
// to catch this... good documentation, guys!
#if defined(__GNUC__) && !(defined (__MINGW) || defined(__MINGW32__))
char* strupr( char * str )
{
    char *ret = str;
    if ( NULL != str )
    {
        while ( CSTR_END != *str )
        {
            *str = Ego::toupper( *str );
            str++;
        }
    }
    return ret;
}

char* strlwr( char * str )
{
    char *ret = str;
    if ( NULL != str )
    {
        while ( CSTR_END != *str )
        {
            *str = Ego::tolower( *str );
            str++;
        }
    }
    return ret;
}

#endif
