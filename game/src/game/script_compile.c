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

/// @file game/script_compile.c
/// @brief Implementation of the script compiler
/// @details

#include "game/script_compile.h"
#include "game/game.h"
#include "game/renderer_2d.h"
#include "game/egoboo.h"
#include "egolib/Log/Entry.hpp"

namespace Log {
struct CompilerEntry : Entry {
	std::string _scriptName;
	int _scriptLine;
	CompilerEntry(Level level, const std::string& file, int line, const std::string& function,
		          const std::string& scriptName, int scriptLine)
		: Entry(level, file, line, function), _scriptName(scriptName), _scriptLine(scriptLine) {
		getSink() << ": " << scriptName << ":" << scriptLine << ": ";
	}
};
}

static bool load_ai_codes_vfs();

parser_state_t::parser_state_t()
	: _token(), _linebuffer()
{
	_line_count = 0;

	_load_buffer_count = 0;
	_load_buffer[0] = '\0';

    load_ai_codes_vfs();
    debug_script_file = vfs_openWrite("/debug/script_debug.txt");

    _error = false;
}

parser_state_t::~parser_state_t()
{
	_line_count = 0;

	_load_buffer_count = 0;
	_load_buffer[0] = '\0';

    vfs_close(debug_script_file);
    debug_script_file = nullptr;

    _error = false;
}

bool parser_state_t::get_error() const
{
    return _error;
}

void parser_state_t::clear_error()
{
    _error = false;
}

/// the pointer to the singleton
parser_state_t * parser_state_t::_singleton = nullptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

StaticArray<opcode_data_t, MAX_OPCODE> OpList;

bool debug_scripts = false;
vfs_FILE *debug_script_file = NULL;

//--------------------------------------------------------------------------------------------

const char *script_variable_names[SCRIPT_VARIABLES_COUNT] =
{
#define Define(name) #name,
#include "egolib/Script/Variables.in"
#undef Define
};
const char *script_function_names[Ego::ScriptFunctions::SCRIPT_FUNCTIONS_COUNT] =
{
#define Define(name) #name,
#include "egolib/Script/Functions.in"
#undef Define
};
const char *script_operator_names[SCRIPT_OPERATORS_COUNT] =
{
#define Define(name) #name,
#include "egolib/Script/Operators.in"
#undef Define
};


//--------------------------------------------------------------------------------------------

/// Emit a token to standard output in debug mode.
void print_token(const Token& token);
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
    static void print_line();
#else
    #define print_line()
#endif

//--------------------------------------------------------------------------------------------

parser_state_t *parser_state_t::initialize()
{
    if (!_singleton)
    {
        _singleton = new parser_state_t();
    }
    return _singleton;
}

void parser_state_t::uninitialize()
{
    if (_singleton)
    {
		delete _singleton;
		_singleton = nullptr;
    }
}

parser_state_t& parser_state_t::get()
{
	if (!_singleton) {
		throw std::logic_error("parser singleton not initialized");
	}
    return *_singleton;
}

//--------------------------------------------------------------------------------------------
static bool isNewline(char x) {
    return ASCII_LINEFEED_CHAR == x || C_CARRIAGE_RETURN_CHAR == x;
}

bool parser_state_t::skipNewline(size_t& read, script_info_t& script) {
    size_t newread = read;
    if (newread < _load_buffer_count) {
        char current = _load_buffer[newread];
        if (isNewline(current)) {
            newread++;
            if (newread < _load_buffer_count) {
                char old = current;
                current = _load_buffer[newread];
                if (isNewline(current) && old != current) {
                    newread++;
                }
            }
        }
    }
    if (read != newread) {
        read = newread;
        return true;
    } else {
        return false;
    }
}
size_t parser_state_t::load_one_line( size_t read, script_info_t& script )
{
    /// @author ZZ
    /// @details This function loads a line into the line buffer

    char cTmp;

    // Parse to start to maintain indentation
	_linebuffer.clear();

    // try to trap all end of line conditions so we can properly count the lines
    bool tabs_warning_needed = false;
    while ( read < _load_buffer_count )
    {
        if (skipNewline(read, script)) {
            _linebuffer.clear();
            return read;
        }

        cTmp = _load_buffer[read];
        if ( C_TAB_CHAR == cTmp )
        {
            tabs_warning_needed = true;
            cTmp = ' ';
        }

        if (!Ego::isspace(cTmp))
        {
            break;
        }

        _linebuffer.append(' ');

        read++;
    }

    // Parse to comment or end of line
    bool foundtext = false;
    bool inside_string = false;
    while ( read < _load_buffer_count )
    {
        cTmp = _load_buffer[read];

        // we reached endline
        if (isNewline(cTmp))
        {
            break;
        }

        // we reached a comment
        if ( '/' == cTmp && '/' == _load_buffer[read + 1] )
        {
            break;
        }

        // Double apostrophe indicates where the string begins and ends
        if ( C_DOUBLE_QUOTE_CHAR == cTmp )
        {
            inside_string = !inside_string;
        }

        if ( inside_string )
        {
            if ( '\t' == cTmp )
            {
                // convert tab characters to the '~' symbol
                cTmp = '~';
            }
            else if (Ego::isspace(cTmp) || Ego::iscntrl(cTmp))
            {
                // all whitespace and control characters are converted to '_'
                cTmp = '_';
            }
        }
        else if (Ego::iscntrl(cTmp))
        {
            // Convert control characters to whitespace
            cTmp = ' ';
        }

        // convert whitespace characters to
        if ( !isspace(( unsigned )cTmp ) || inside_string )
        {
            foundtext = true;

            _linebuffer.append(cTmp);
        }

        read++;
    }

    if ( !foundtext )
    {
        _linebuffer.clear();
    }

    if ( _linebuffer.size() > 0  && tabs_warning_needed )
    {
		Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), _token.getLine());
		e << "compilation error - tab character used to define spacing will cause an error `"
		  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;
    }

    // scan to the beginning of the next line
    while ( read < _load_buffer_count )
    {
        if (skipNewline(read, script)) {
            break;
        } else if (CSTR_END == _load_buffer[read]) {
            read += 1;
            break;
        }

        read++;
    }

    return read;
}

//--------------------------------------------------------------------------------------------
void parser_state_t::surround_space( size_t position, linebuffer_t& buffer )
{
    if (!Ego::isspace(buffer[position + 1]))
    {
        buffer.insert(' ', position + 1);
    }
    if (!Ego::isspace(buffer[position + 0]))
    {
        buffer.insert(' ', position + 0);
    }
}

//--------------------------------------------------------------------------------------------
int parser_state_t::get_indentation(script_info_t& script )
{
    int cnt = 0;
    char cTmp = _linebuffer[cnt];
    while (Ego::isspace(cTmp))
    {
        cnt++;
        cTmp = _linebuffer[cnt];
    }
    if ( HAS_SOME_BITS( cnt, 1 ) )
    {
		Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), _token.getLine());
		e << "invalid indention - number of spaces must be even - \n"
		  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;
        _error = true;
    }

    cnt >>= 1;
    if ( cnt > 15 )
    {
		Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), _token.getLine());
		e << "invalid indention - too many spaces - \n"
	      << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;
        _error = true;
        cnt = 15;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
size_t parser_state_t::fix_operators( linebuffer_t& buffer )
{
    /// @author ZZ
    /// @details This function puts spaces around operators to seperate words better

    bool inside_string = false;

    size_t cnt = 0;
    while ( cnt < buffer.size() )
    {
        char cTmp = buffer[cnt];
        if ( C_DOUBLE_QUOTE_CHAR == cTmp )
        {
            inside_string = !inside_string;
        }

        //Don't fix operator symbols inside a string
        if ( !inside_string )
        {
            if ( '+' == cTmp || '-' == cTmp || '/' == cTmp || '*' == cTmp ||
                 '%' == cTmp || '>' == cTmp || '<' == cTmp || '&' == cTmp ||
                 '=' == cTmp )
            {
                surround_space( cnt, buffer);
                cnt++;
            }
        }

        cnt++;
    }

    return buffer.size();
}

//--------------------------------------------------------------------------------------------
size_t parser_state_t::parse_token(Token& tok, ObjectProfile *ppro, script_info_t& script, size_t read)
{
    /// @author ZZ
    /// @details This function tells what code is being indexed by read, it
    ///    will return the next spot to read from and stick the code number
    ///    in ptok->iIndex

    int cnt;

    // figure out what the max word length actually is
    const size_t szWord_length_max = SDL_arraysize( tok.szWord );

    // Reset the token
	tok = Token();

    // Check bounds
	if ( read >= _linebuffer.size() )
    {
        return _linebuffer.size();
    }

    // nothing is parsed yet
    bool parsed = false;

    // Skip any initial spaces
    char cTmp = _linebuffer[read];
    while (Ego::isspace(cTmp) && read < _linebuffer.size())
    {
        read++;
        cTmp = _linebuffer[read];
    }

    // break if there was nothing here
    if ( read >= _linebuffer.size())
    {
        goto parse_token_end;
    }

    // initialize the word
    tok.szWord_length = 0;
    tok.szWord[0] = CSTR_END;

    // handle the special case of a string constant
    if ( C_DOUBLE_QUOTE_CHAR == cTmp )
    {
        do
        {
            // begin the copy
            tok.szWord[tok.szWord_length] = cTmp;
            tok.szWord_length++;

            read++;
            cTmp = _linebuffer[read];

            // Break out if we find the end of the string
            // Strings of the form "Here lies \"The Sandwich King\"" are not supported
        }
        while ( CSTR_END != cTmp && C_DOUBLE_QUOTE_CHAR != cTmp && tok.szWord_length < szWord_length_max && read < _linebuffer.size());

        if ( C_DOUBLE_QUOTE_CHAR == cTmp )
        {
            // skip the ending qoutation mark
            read++;
            cTmp = _linebuffer[read];

            tok.szWord[tok.szWord_length] = CSTR_END;
            tok.szWord_length++;
        }
        else
        {
			Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), tok.getLine());
			e << "string literal is too long - \n"
	          << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
			Log::get() << e;
        }
    }
    else
    {
        // Load the the word into the ptok->szWord buffer
        tok.szWord_length = 0;
        tok.szWord[0] = CSTR_END;

        while (!Ego::isspace(cTmp) && CSTR_END != cTmp && tok.szWord_length < szWord_length_max && read < _linebuffer.size())
        {
            tok.szWord[tok.szWord_length] = cTmp;
            tok.szWord_length++;

            read++;
            cTmp = _linebuffer[read];
        }

        if ( tok.szWord_length < szWord_length_max )
        {
            tok.szWord[tok.szWord_length] = CSTR_END;
        }
    }

    // ensure that the string is terminated
    tok.szWord[szWord_length_max-1] = CSTR_END;

    // Check for numeric constant
    if (!parsed && Ego::isdigit(tok.szWord[0]))
    {
		int temporary;
		sscanf(tok.szWord, "%d", &temporary);
		tok.setValue(temporary);
        tok.setType(Token::Type::Constant);
        tok.setIndex(MAX_OPCODE);

        // move on to the next thing
        parsed = true;
    }

    // Check for IDSZ constant
    if ( !parsed && ( '[' == tok.szWord[0] ) )
    {
        IDSZ idsz = MAKE_IDSZ( tok.szWord[1], tok.szWord[2], tok.szWord[3], tok.szWord[4] );

        tok.setValue(idsz);
        tok.setType(Token::Type::Constant);
        tok.setIndex(MAX_OPCODE);

        // move on to the next thing
        parsed = true;
    }

    if ( !parsed && ( 0 == strcmp( tok.szWord, "=" ) ) )
    {
        tok.setValue(-1);
        tok.setType(Token::Type::Operator);
        tok.setIndex(MAX_OPCODE);

        // move on to the next thing
        parsed = true;
    }

    // convert the string token to a new token type
    if ( !parsed && ( C_DOUBLE_QUOTE_CHAR == tok.szWord[0] ) )
    {
        char * str = tok.szWord + 1;

        if ( CSTR_END == tok.szWord[1] || C_DOUBLE_QUOTE_CHAR == tok.szWord[1] )
        {
            // some kind of empty string
			Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), tok.getLine());
			e << "string literal is empty - \n"
			  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
			Log::get() << e;

            // some kind of error
            parsed = true;
        }
        else if ( '#' == str[0] )
        {
            //remove the reference symbol to figure out the actual folder name we are looking for
            std::string obj_name = str + 1;

            // Invalid profile as default
            tok.setValue(INVALID_PRO_REF);

            // Convert reference to slot number
            for (const auto &element : ProfileSystem::get().getLoadedProfiles())
            {
                const std::shared_ptr<ObjectProfile> &profile = element.second;
                if(profile == nullptr) continue;


                //is this the object we are looking for?
                if (Ego::isSuffix(profile->getPathname(), obj_name))
                {
                    tok.setValue(profile->getSlotNumber());
                    break;
                }
            }

            // Do we need to load the object?
            if (!ProfileSystem::get().isValidProfileID((PRO_REF)tok.getValue()))
            {
                std::string loadname = "mp_objects/" + obj_name;

                //find first free slot number
                for (PRO_REF ipro = MAX_IMPORT_PER_PLAYER * 4; ipro < INVALID_PRO_REF; ipro++ )
                {
                    //skip loaded profiles
                    if (ProfileSystem::get().isValidProfileID(ipro)) continue;

                    //found a free slot
                    tok.setValue(ProfileSystem::get().loadOneProfile(loadname, REF_TO_INT(ipro)));
                    if (tok.getValue() == ipro) break;
                }
            }

            // Failed to load object!
            if (!ProfileSystem::get().isValidProfileID((PRO_REF)tok.getValue()))
            {
				Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), tok.getLine());
				e << "failed to load object " << tok.szWord << " - \n"
				  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
				Log::get() << e;
            }

            tok.setType(Token::Type::Constant);
            tok.setIndex(MAX_OPCODE);

            parsed = true;
        }
        else
        {
            // a normal string
            // if this is a new string, add this message to the avalible messages of the object
            tok.setValue(ppro->addMessage(str, true));

            tok.setType(Token::Type::Constant);
            tok.setIndex(MAX_OPCODE);

            parsed = true;
        }
    }

    // is it a constant, opcode, or value?
    if ( !parsed )
    {
        for ( cnt = 0; cnt < OpList.count; cnt++ )
        {
            if ( 0 == strncmp( tok.szWord, OpList.ary[cnt].cName, MAXCODENAMESIZE ) )
            {
                tok.setValue(OpList.ary[cnt].iValue);
                tok.setType(OpList.ary[cnt]._type);
                tok.setIndex(cnt);

                // move on to the next thing
                parsed = true;

                break;
            }
        }
    }

    // We couldn't figure out what this is, throw out an error code
    if ( !parsed )
    {
		Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), tok.getLine());
		e << "unknown opcode " << tok.szWord << " - \n"
		  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;

        // put the token in an error state
        tok.setValue(-1);
        tok.setType(Token::Type::Unknown);
        tok.setIndex(MAX_OPCODE);

        _error = true;
    }

parse_token_end:

    print_token(tok);
    return read;
}

//--------------------------------------------------------------------------------------------
void parser_state_t::emit_opcode( Token& tok, const BIT_FIELD highbits, script_info_t& script )
{
    BIT_FIELD loc_highbits = highbits;

    // emit constant or a function
    if ( Token::Type::Constant == tok.getType() || Token::Type::Function == tok.getType() )
    {
        SET_BIT( loc_highbits, Instruction::FUNCTIONBITS );
    }

    // emit the opcode
    if (!script._instructions.isFull())
    {
		script._instructions.append(Instruction(loc_highbits | tok.getValue()));
    }
    else
    {
		Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), tok.getLine());
		e << "script index larger than array - \n"
		  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;
    }

}

//--------------------------------------------------------------------------------------------
void parser_state_t::parse_line_by_line( ObjectProfile *ppro, script_info_t& script )
{
    /// @author ZF
    /// @details This parses an AI script line by line

    size_t read = 0;
    for ( _token.setLine(0); read < _load_buffer_count; _token.setLine(_token.getLine() + 1) )
    {
        read = load_one_line( read, script );
        if ( 0 == _linebuffer.size() ) continue;

#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
        print_line();
#endif

        fix_operators( _linebuffer );

        //------------------------------
        // grab the first opcode

        uint32_t highbits = SetDataBits( get_indentation( script ) );
        size_t parseposition = parse_token(_token, ppro, script, 0 );
        if ( Token::Type::Function == _token.getType() )
        {
            if ( Ego::ScriptFunctions::End == _token.getValue() && 0 == highbits )
            {
                // stop processing the lines, since we're finished
                break;
            }

            //------------------------------
            // the code type is a function

            // save the opcode
            emit_opcode( _token, highbits, script );

            // leave a space for the control code
            _token.setValue(0);
            emit_opcode( _token, 0, script );

        }
        else if ( Token::Type::Variable == _token.getType() )
        {
            //------------------------------
            // the code type is a math operation

            int operand_index;
            int operands = 0;

            // save in the value's opcode
            emit_opcode( _token, highbits, script );

            // save a position for the operand count
            _token.setValue(0);
            operand_index = script._instructions.getLength();    //AisCompiled_offset;
            emit_opcode( _token, 0, script );

            // handle the "="
            highbits = 0;
            parseposition = parse_token(_token, ppro, script, parseposition );  // EQUALS
			if ( Token::Type::Operator != _token.getType() || 0 != strcmp( _token.szWord, "=" ) )
            {
				Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), _token.getLine());
				e << "invalid equation - \n"
				  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
            }

            //------------------------------
            // grab the next opcode

            parseposition = parse_token( _token, ppro, script, parseposition );
            if ( Token::Type::Variable == _token.getType() || Token::Type::Constant == _token.getType() )
            {
                // this is a value or a constant
                emit_opcode( _token, 0, script );
                operands++;

                parseposition = parse_token(_token, ppro, script, parseposition );
            }
            else if ( Token::Type::Operator != _token.getType() )
            {
                // this is a function or an unknown value. do not break the script.
				Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), _token.getLine());
				e << "invalid operand " << _token.szWord << " - \n"
				  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
				Log::get() << e;

                emit_opcode( _token, 0, script );
                operands++;

                parseposition = parse_token(_token, ppro, script, parseposition );
            }

            // expects a OPERATOR VALUE OPERATOR VALUE OPERATOR VALUE pattern
            while ( parseposition < _linebuffer.size() )
            {
                // the current token should be an operator
                if ( Token::Type::Operator != _token.getType() )
                {
                    // problem with the loop
					Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, script.getName(), _token.getLine());
					e << "expected operator - \n"
					  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
					Log::get() << e;
                    break;
                }

                // the highbits are the operator's value
				highbits = SetDataBits( _token.getValue() );

                // VALUE
                parseposition = parse_token(_token, ppro, script, parseposition );
                if ( Token::Type::Constant != _token.getType() && Token::Type::Variable != _token.getType() )
                {
                    // not having a constant or a value here breaks the function. stop processing
					Log::get().message("%s:%d:%s: compilation error - invalid operand \"%s\"(%d) - \"%s\"\n", \
						               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.szWord );
                    break;
                }

                emit_opcode( _token, highbits, script );
                operands++;

                // OPERATOR
                parseposition = parse_token( _token, ppro, script, parseposition );
            }
            script._instructions[operand_index]._value = operands;
        }
        else if ( Token::Type::Constant == _token.getType() )
        {
			Log::get().message("%s:%d:%s: compilation error - invalid constant \"%s\"(%d) - \"%s\"\n", \
				               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.szWord );
        }
        else if ( Token::Type::Unknown == _token.getType() )
        {
            // unknown opcode, do not process this line
			Log::get().message("%s:%d:%s: compilation error - invalid operand \"%s\"(%d) - \"%s\"\n", \
				               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.szWord );
        }
        else
        {
			Log::get().message("%s:%d:%s: compilation error - compiler is broken \"%s\"(%d) - \"%s\"\n", \
				               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.szWord );
            break;
        }
    }

    _token.setValue(Ego::ScriptFunctions::End);
    _token.setType(Token::Type::Function);
    emit_opcode( _token, 0, script );
    _token.setValue(script._instructions.getLength() + 1);
    emit_opcode( _token, 0, script );
}

//--------------------------------------------------------------------------------------------
Uint32 parser_state_t::jump_goto( int index, int index_end, script_info_t& script )
{
    /// @author ZZ
    /// @details This function figures out where to jump to on a fail based on the
    ///    starting location and the following code.  The starting location
    ///    should always be a function code with indentation

    auto value = script._instructions[index]; /*AisCompiled_buffer[index];*/  index += 2;
    int targetindent = GetDataBits( value._value );
    int indent = 100;

    while ( indent > targetindent && index < index_end )
    {
        value = script._instructions[index]; //AisCompiled_buffer[index];
        indent = GetDataBits ( value._value );
        if ( indent > targetindent )
        {
            // Was it a function
            if ( value.isInv() )
            {
                // Each function needs a jump
                index++;
                index++;
            }
            else
            {
                // Operations cover each operand
                index++;
                value = script._instructions[index]; //AisCompiled_buffer[index];
                index++;
                index += ( value._value & 255 );
            }
        }
    }

    return std::min( index, index_end );
}

//--------------------------------------------------------------------------------------------
void parser_state_t::parse_jumps( script_info_t& script )
{
    /// @author ZZ
    /// @details This function sets up the fail jumps for the down and dirty code

    uint32_t index     = 0;
    uint32_t index_end = script._instructions.getLength();

    auto value = script._instructions[index];
    while ( index < index_end )
    {
        value = script._instructions[index];

        // Was it a function
        if (value.isInv())
        {
            // Each function needs a jump
            auto iTmp = jump_goto( index, index_end, script );
            index++;
            script._instructions[index]._value = iTmp;              //AisCompiled_buffer[index] = iTmp;
            index++;
        }
        else
        {
            // Operations cover each operand
            index++;
            auto iTmp = script._instructions[index];              //AisCompiled_buffer[index];
            index++;
			index += CLIP_TO_08BITS( iTmp._value );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool load_ai_codes_vfs()
{
    /// @author ZZ
    /// @details This function loads all of the function and variable names

	struct aicode_t
	{
		/// The type.
		Token::Type _type;
		/// The value of th constant.
		uint32_t _value;
		/// The name.
		const char *_name;
	};

	static const aicode_t AICODES[] =
	{
		{ Token::Type::Function, 0, "IfSpawned" },
		{ Token::Type::Function, 1, "IfTimeOut" },
		{ Token::Type::Function, 2, "IfAtWaypoint" },
		{ Token::Type::Function, 3, "IfAtLastWaypoint" },
		{ Token::Type::Function, 3, "IfPutAway" },
		{ Token::Type::Function, 4, "IfAttacked" },
		{ Token::Type::Function, 5, "IfBumped" },
		{ Token::Type::Function, 6, "IfOrdered" },
		{ Token::Type::Function, 7, "IfCalledForHelp" },
		{ Token::Type::Function, 8, "SetContent" },
		{ Token::Type::Function, 9, "IfKilled" },
		{ Token::Type::Function, 10, "IfTargetKilled" },
		{ Token::Type::Function, 11, "ClearWaypoints" },
		{ Token::Type::Function, 12, "AddWaypoint" },
		{ Token::Type::Function, 13, "FindPath" },
		{ Token::Type::Function, 14, "Compass" },
		{ Token::Type::Function, 15, "GetTargetArmorPrice" },
		{ Token::Type::Function, 16, "SetTime" },
		{ Token::Type::Function, 17, "GetContent" },
		{ Token::Type::Function, 18, "JoinTargetTeam" },
		{ Token::Type::Function, 19, "SetTargetToNearbyEnemy" },
		{ Token::Type::Function, 20, "SetTargetToTargetLeftHand" },
		{ Token::Type::Function, 21, "SetTargetToTargetRightHand" },
		{ Token::Type::Function, 22, "SetTargetToWhoeverAttacked" },
		{ Token::Type::Function, 22, "SetTargetToWhoeverHealed" },
		{ Token::Type::Function, 23, "SetTargetToWhoeverBumped" },
		{ Token::Type::Function, 24, "SetTargetToWhoeverCalledForHelp" },
		{ Token::Type::Function, 25, "SetTargetToOldTarget" },
		{ Token::Type::Function, 26, "SetTurnModeToVelocity" },
		{ Token::Type::Function, 27, "SetTurnModeToWatch" },
		{ Token::Type::Function, 28, "SetTurnModeToSpin" },
		{ Token::Type::Function, 29, "SetBumpHeight" },
		{ Token::Type::Function, 30, "IfTargetHasID" },
		{ Token::Type::Function, 31, "IfTargetHasItemID" },
		{ Token::Type::Function, 32, "IfTargetHoldingItemID" },
		{ Token::Type::Function, 33, "IfTargetHasSkillID" },
		{ Token::Type::Function, 34, "Else" },
		{ Token::Type::Function, 35, "Run" },
		{ Token::Type::Function, 36, "Walk" },
		{ Token::Type::Function, 37, "Sneak" },
		{ Token::Type::Function, 38, "DoAction" },
		{ Token::Type::Function, 39, "KeepAction" },
		{ Token::Type::Function, 40, "IssueOrder" },
		{ Token::Type::Function, 41, "DropWeapons" },
		{ Token::Type::Function, 42, "TargetDoAction" },
		{ Token::Type::Function, 43, "OpenPassage" },
		{ Token::Type::Function, 44, "ClosePassage" },
		{ Token::Type::Function, 45, "IfPassageOpen" },
		{ Token::Type::Function, 46, "GoPoof" },
		{ Token::Type::Function, 47, "CostTargetItemID" },
		{ Token::Type::Function, 48, "DoActionOverride" },
		{ Token::Type::Function, 49, "IfHealed" },
		{ Token::Type::Function, 50, "SendMessage" },
		{ Token::Type::Function, 51, "CallForHelp" },
		{ Token::Type::Function, 52, "AddIDSZ" },
		{ Token::Type::Function, 53, "End" },
		{ Token::Type::Function, 54, "SetState" },
		{ Token::Type::Function, 55, "GetState" },
		{ Token::Type::Function, 56, "IfStateIs" },
		{ Token::Type::Function, 57, "IfTargetCanOpenStuff" },
		{ Token::Type::Function, 58, "IfGrabbed" },
		{ Token::Type::Function, 58, "IfMounted" },
		{ Token::Type::Function, 59, "IfDropped" },
		{ Token::Type::Function, 59, "IfDismounted" },
		{ Token::Type::Function, 60, "SetTargetToWhoeverIsHolding" },
		{ Token::Type::Function, 61, "DamageTarget" },
		{ Token::Type::Function, 62, "IfXIsLessThanY" },
		{ Token::Type::Function, 62, "IfYIsMoreThanX" },
		{ Token::Type::Function, 63, "SetWeatherTime" },
		{ Token::Type::Function, 64, "GetBumpHeight" },
		{ Token::Type::Function, 65, "IfReaffirmed" },
		{ Token::Type::Function, 66, "UnkeepAction" },
		{ Token::Type::Function, 67, "IfTargetIsOnOtherTeam" },
		{ Token::Type::Function, 68, "IfTargetIsOnHatedTeam" },
		{ Token::Type::Function, 69, "PressLatchButton" },
		{ Token::Type::Function, 70, "SetTargetToTargetOfLeader" },
		{ Token::Type::Function, 71, "IfLeaderKilled" },
		{ Token::Type::Function, 72, "BecomeLeader" },
		{ Token::Type::Function, 73, "ChangeTargetArmor" },
		{ Token::Type::Function, 74, "GiveMoneyToTarget" },
		{ Token::Type::Function, 75, "DropKeys" },
		{ Token::Type::Function, 76, "IfLeaderIsAlive" },
		{ Token::Type::Function, 77, "IfTargetIsOldTarget" },
		{ Token::Type::Function, 78, "SetTargetToLeader" },
		{ Token::Type::Function, 79, "SpawnCharacter" },
		{ Token::Type::Function, 80, "RespawnCharacter" },
		{ Token::Type::Function, 81, "ChangeTile" },
		{ Token::Type::Function, 82, "IfUsed" },
		{ Token::Type::Function, 83, "DropMoney" },
		{ Token::Type::Function, 84, "SetOldTarget" },
		{ Token::Type::Function, 85, "DetachFromHolder" },
		{ Token::Type::Function, 86, "IfTargetHasVulnerabilityID" },
		{ Token::Type::Function, 87, "CleanUp" },
		{ Token::Type::Function, 88, "IfCleanedUp" },
		{ Token::Type::Function, 89, "IfSitting" },
		{ Token::Type::Function, 89, "IfHeld" },
		{ Token::Type::Function, 90, "IfTargetIsHurt" },
		{ Token::Type::Function, 91, "IfTargetIsAPlayer" },
		{ Token::Type::Function, 92, "PlaySound" },
		{ Token::Type::Function, 93, "SpawnParticle" },
		{ Token::Type::Function, 94, "IfTargetIsAlive" },
		{ Token::Type::Function, 95, "Stop" },
		{ Token::Type::Function, 96, "DisaffirmCharacter" },
		{ Token::Type::Function, 97, "ReaffirmCharacter" },
		{ Token::Type::Function, 98, "IfTargetIsSelf" },
		{ Token::Type::Function, 99, "IfTargetIsMale" },
		{ Token::Type::Function, 100, "IfTargetIsFemale" },
		{ Token::Type::Function, 101, "SetTargetToSelf" },
		{ Token::Type::Function, 102, "SetTargetToRider" },
		{ Token::Type::Function, 103, "GetAttackTurn" },
		{ Token::Type::Function, 104, "GetDamageType" },
		{ Token::Type::Function, 105, "BecomeSpell" },
		{ Token::Type::Function, 106, "BecomeSpellbook" },
		{ Token::Type::Function, 107, "IfScoredAHit" },
		{ Token::Type::Function, 108, "IfDisaffirmed" },
		{ Token::Type::Function, 109, "TranslateOrder" },
		{ Token::Type::Function, 110, "SetTargetToWhoeverWasHit" },
		{ Token::Type::Function, 111, "SetTargetToWideEnemy" },
		{ Token::Type::Function, 112, "IfChanged" },
		{ Token::Type::Function, 113, "IfInWater" },
		{ Token::Type::Function, 114, "IfBored" },
		{ Token::Type::Function, 115, "IfTooMuchBaggage" },
		{ Token::Type::Function, 116, "IfGrogged" },
		{ Token::Type::Function, 117, "IfDazed" },
		{ Token::Type::Function, 118, "IfTargetHasSpecialID" },
		{ Token::Type::Function, 119, "PressTargetLatchButton" },
		{ Token::Type::Function, 120, "IfInvisible" },
		{ Token::Type::Function, 121, "IfArmorIs" },
		{ Token::Type::Function, 122, "GetTargetGrogTime" },
		{ Token::Type::Function, 123, "GetTargetDazeTime" },
		{ Token::Type::Function, 124, "SetDamageType" },
		{ Token::Type::Function, 125, "SetWaterLevel" },
		{ Token::Type::Function, 126, "EnchantTarget" },
		{ Token::Type::Function, 127, "EnchantChild" },
		{ Token::Type::Function, 128, "TeleportTarget" },
		{ Token::Type::Function, 129, "GiveExperienceToTarget" },
		{ Token::Type::Function, 130, "IncreaseAmmo" },
		{ Token::Type::Function, 131, "UnkurseTarget" },
		{ Token::Type::Function, 132, "GiveExperienceToTargetTeam" },
		{ Token::Type::Function, 133, "IfUnarmed" },
		{ Token::Type::Function, 134, "RestockTargetAmmoIDAll" },
		{ Token::Type::Function, 135, "RestockTargetAmmoIDFirst" },
		{ Token::Type::Function, 136, "FlashTarget" },
		{ Token::Type::Function, 137, "SetRedShift" },
		{ Token::Type::Function, 138, "SetGreenShift" },
		{ Token::Type::Function, 139, "SetBlueShift" },
		{ Token::Type::Function, 140, "SetLight" },
		{ Token::Type::Function, 141, "SetAlpha" },
		{ Token::Type::Function, 142, "IfHitFromBehind" },
		{ Token::Type::Function, 143, "IfHitFromFront" },
		{ Token::Type::Function, 144, "IfHitFromLeft" },
		{ Token::Type::Function, 145, "IfHitFromRight" },
		{ Token::Type::Function, 146, "IfTargetIsOnSameTeam" },
		{ Token::Type::Function, 147, "KillTarget" },
		{ Token::Type::Function, 148, "UndoEnchant" },
		{ Token::Type::Function, 149, "GetWaterLevel" },
		{ Token::Type::Function, 150, "CostTargetMana" },
		{ Token::Type::Function, 151, "IfTargetHasAnyID" },
		{ Token::Type::Function, 152, "SetBumpSize" },
		{ Token::Type::Function, 153, "IfNotDropped" },
		{ Token::Type::Function, 154, "IfYIsLessThanX" },
		{ Token::Type::Function, 154, "IfXIsMoreThanY" },
		{ Token::Type::Function, 155, "SetFlyHeight" },
		{ Token::Type::Function, 156, "IfBlocked" },
		{ Token::Type::Function, 157, "IfTargetIsDefending" },
		{ Token::Type::Function, 158, "IfTargetIsAttacking" },
		{ Token::Type::Function, 159, "IfStateIs0" },
		{ Token::Type::Function, 159, "IfStateIsParry" },
		{ Token::Type::Function, 160, "IfStateIs1" },
		{ Token::Type::Function, 160, "IfStateIsWander" },
		{ Token::Type::Function, 161, "IfStateIs2" },
		{ Token::Type::Function, 161, "IfStateIsGuard" },
		{ Token::Type::Function, 162, "IfStateIs3" },
		{ Token::Type::Function, 162, "IfStateIsFollow" },
		{ Token::Type::Function, 163, "IfStateIs4" },
		{ Token::Type::Function, 163, "IfStateIsSurround" },
		{ Token::Type::Function, 164, "IfStateIs5" },
		{ Token::Type::Function, 164, "IfStateIsRetreat" },
		{ Token::Type::Function, 165, "IfStateIs6" },
		{ Token::Type::Function, 165, "IfStateIsCharge" },
		{ Token::Type::Function, 166, "IfStateIs7" },
		{ Token::Type::Function, 166, "IfStateIsCombat" },
		{ Token::Type::Function, 167, "IfContentIs" },
		{ Token::Type::Function, 168, "SetTurnModeToWatchTarget" },
		{ Token::Type::Function, 169, "IfStateIsNot" },
		{ Token::Type::Function, 170, "IfXIsEqualToY" },
		{ Token::Type::Function, 170, "IfYIsEqualToX" },
		{ Token::Type::Function, 171, "DebugMessage" },
		{ Token::Type::Function, 172, "BlackTarget" },
		{ Token::Type::Function, 173, "SendMessageNear" },
		{ Token::Type::Function, 174, "IfHitGround" },
		{ Token::Type::Function, 175, "IfNameIsKnown" },
		{ Token::Type::Function, 176, "IfUsageIsKnown" },
		{ Token::Type::Function, 177, "IfHoldingItemID" },
		{ Token::Type::Function, 178, "IfHoldingRangedWeapon" },
		{ Token::Type::Function, 179, "IfHoldingMeleeWeapon" },
		{ Token::Type::Function, 180, "IfHoldingShield" },
		{ Token::Type::Function, 181, "IfKursed" },
		{ Token::Type::Function, 182, "IfTargetIsKursed" },
		{ Token::Type::Function, 183, "IfTargetIsDressedUp" },
		{ Token::Type::Function, 184, "IfOverWater" },
		{ Token::Type::Function, 185, "IfThrown" },
		{ Token::Type::Function, 186, "MakeNameKnown" },
		{ Token::Type::Function, 187, "MakeUsageKnown" },
		{ Token::Type::Function, 188, "StopTargetMovement" },
		{ Token::Type::Function, 189, "SetXY" },
		{ Token::Type::Function, 190, "GetXY" },
		{ Token::Type::Function, 191, "AddXY" },
		{ Token::Type::Function, 192, "MakeAmmoKnown" },
		{ Token::Type::Function, 193, "SpawnAttachedParticle" },
		{ Token::Type::Function, 194, "SpawnExactParticle" },
		{ Token::Type::Function, 195, "AccelerateTarget" },
		{ Token::Type::Function, 196, "IfDistanceIsMoreThanTurn" },
		{ Token::Type::Function, 197, "IfCrushed" },
		{ Token::Type::Function, 198, "MakeCrushValid" },
		{ Token::Type::Function, 199, "SetTargetToLowestTarget" },
		{ Token::Type::Function, 200, "IfNotPutAway" },
		{ Token::Type::Function, 200, "IfNotTakenOut" },
		{ Token::Type::Function, 201, "IfTakenOut" },
		{ Token::Type::Function, 202, "IfAmmoOut" },
		{ Token::Type::Function, 203, "PlaySoundLooped" },
		{ Token::Type::Function, 204, "StopSound" },
		{ Token::Type::Function, 205, "HealSelf" },
		{ Token::Type::Function, 206, "Equip" },
		{ Token::Type::Function, 207, "IfTargetHasItemIDEquipped" },
		{ Token::Type::Function, 208, "SetOwnerToTarget" },
		{ Token::Type::Function, 209, "SetTargetToOwner" },
		{ Token::Type::Function, 210, "SetFrame" },
		{ Token::Type::Function, 211, "BreakPassage" },
		{ Token::Type::Function, 212, "SetReloadTime" },
		{ Token::Type::Function, 213, "SetTargetToWideBlahID" },
		{ Token::Type::Function, 214, "PoofTarget" },
		{ Token::Type::Function, 215, "ChildDoActionOverride" },
		{ Token::Type::Function, 216, "SpawnPoof" },
		{ Token::Type::Function, 217, "SetSpeedPercent" },
		{ Token::Type::Function, 218, "SetChildState" },
		{ Token::Type::Function, 219, "SpawnAttachedSizedParticle" },
		{ Token::Type::Function, 220, "ChangeArmor" },
		{ Token::Type::Function, 221, "ShowTimer" },
		{ Token::Type::Function, 222, "IfFacingTarget" },
		{ Token::Type::Function, 223, "PlaySoundVolume" },
		{ Token::Type::Function, 224, "SpawnAttachedFacedParticle" },
		{ Token::Type::Function, 225, "IfStateIsOdd" },
		{ Token::Type::Function, 226, "SetTargetToDistantEnemy" },
		{ Token::Type::Function, 227, "Teleport" },
		{ Token::Type::Function, 228, "GiveStrengthToTarget" },
		{ Token::Type::Function, 229, "GiveIntellectToTarget" },
		{ Token::Type::Function, 230, "GiveIntelligenceToTarget" },
		{ Token::Type::Function, 231, "GiveDexterityToTarget" },
		{ Token::Type::Function, 232, "GiveLifeToTarget" },
		{ Token::Type::Function, 233, "GiveManaToTarget" },
		{ Token::Type::Function, 234, "ShowMap" },
		{ Token::Type::Function, 235, "ShowYouAreHere" },
		{ Token::Type::Function, 236, "ShowBlipXY" },
		{ Token::Type::Function, 237, "HealTarget" },
		{ Token::Type::Function, 238, "PumpTarget" },
		{ Token::Type::Function, 239, "CostAmmo" },
		{ Token::Type::Function, 240, "MakeSimilarNamesKnown" },
		{ Token::Type::Function, 241, "SpawnAttachedHolderParticle" },
		{ Token::Type::Function, 242, "SetTargetReloadTime" },
		{ Token::Type::Function, 243, "SetFogLevel" },
		{ Token::Type::Function, 244, "GetFogLevel" },
		{ Token::Type::Function, 245, "SetFogTAD" },
		{ Token::Type::Function, 246, "SetFogBottomLevel" },
		{ Token::Type::Function, 247, "GetFogBottomLevel" },
		{ Token::Type::Function, 248, "CorrectActionForHand" },
		{ Token::Type::Function, 249, "IfTargetIsMounted" },
		{ Token::Type::Function, 250, "SparkleIcon" },
		{ Token::Type::Function, 251, "UnsparkleIcon" },
		{ Token::Type::Function, 252, "GetTileXY" },
		{ Token::Type::Function, 253, "SetTileXY" },
		{ Token::Type::Function, 254, "SetShadowSize" },
		{ Token::Type::Function, 255, "OrderTarget" },
		{ Token::Type::Function, 256, "SetTargetToWhoeverIsInPassage" },
		{ Token::Type::Function, 257, "IfCharacterWasABook" },
		{ Token::Type::Function, 258, "SetEnchantBoostValues" },
		{ Token::Type::Function, 259, "SpawnCharacterXYZ" },
		{ Token::Type::Function, 260, "SpawnExactCharacterXYZ" },
		{ Token::Type::Function, 261, "ChangeTargetClass" },
		{ Token::Type::Function, 262, "PlayFullSound" },
		{ Token::Type::Function, 263, "SpawnExactChaseParticle" },
		{ Token::Type::Function, 264, "CreateOrder" },
		{ Token::Type::Function, 265, "OrderSpecialID" },
		{ Token::Type::Function, 266, "UnkurseTargetInventory" },
		{ Token::Type::Function, 267, "IfTargetIsSneaking" },
		{ Token::Type::Function, 268, "DropItems" },
		{ Token::Type::Function, 269, "RespawnTarget" },
		{ Token::Type::Function, 270, "TargetDoActionSetFrame" },
		{ Token::Type::Function, 271, "IfTargetCanSeeInvisible" },
		{ Token::Type::Function, 272, "SetTargetToNearestBlahID" },
		{ Token::Type::Function, 273, "SetTargetToNearestEnemy" },
		{ Token::Type::Function, 274, "SetTargetToNearestFriend" },
		{ Token::Type::Function, 275, "SetTargetToNearestLifeform" },
		{ Token::Type::Function, 276, "FlashPassage" },
		{ Token::Type::Function, 277, "FindTileInPassage" },
		{ Token::Type::Function, 278, "IfHeldInLeftHand" },
		{ Token::Type::Function, 279, "NotAnItem" },
		{ Token::Type::Function, 280, "SetChildAmmo" },
		{ Token::Type::Function, 281, "IfHitVulnerable" },
		{ Token::Type::Function, 282, "IfTargetIsFlying" },
		{ Token::Type::Function, 283, "IdentifyTarget" },
		{ Token::Type::Function, 284, "BeatModule" },
		{ Token::Type::Function, 285, "EndModule" },
		{ Token::Type::Function, 286, "DisableExport" },
		{ Token::Type::Function, 287, "EnableExport" },
		{ Token::Type::Function, 288, "GetTargetState" },
		{ Token::Type::Function, 289, "IfEquipped" },
		{ Token::Type::Function, 290, "DropTargetMoney" },
		{ Token::Type::Function, 291, "GetTargetContent" },
		{ Token::Type::Function, 292, "DropTargetKeys" },
		{ Token::Type::Function, 293, "JoinTeam" },
		{ Token::Type::Function, 294, "TargetJoinTeam" },
		{ Token::Type::Function, 295, "ClearMusicPassage" },
		{ Token::Type::Function, 296, "ClearEndMessage" },
		{ Token::Type::Function, 297, "AddEndMessage" },
		{ Token::Type::Function, 298, "PlayMusic" },
		{ Token::Type::Function, 299, "SetMusicPassage" },
		{ Token::Type::Function, 300, "MakeCrushInvalid" },
		{ Token::Type::Function, 301, "StopMusic" },
		{ Token::Type::Function, 302, "FlashVariable" },
		{ Token::Type::Function, 303, "AccelerateUp" },
		{ Token::Type::Function, 304, "FlashVariableHeight" },
		{ Token::Type::Function, 305, "SetDamageTime" },
		{ Token::Type::Function, 306, "IfStateIs8" },
		{ Token::Type::Function, 307, "IfStateIs9" },
		{ Token::Type::Function, 308, "IfStateIs10" },
		{ Token::Type::Function, 309, "IfStateIs11" },
		{ Token::Type::Function, 310, "IfStateIs12" },
		{ Token::Type::Function, 311, "IfStateIs13" },
		{ Token::Type::Function, 312, "IfStateIs14" },
		{ Token::Type::Function, 313, "IfStateIs15" },
		{ Token::Type::Function, 314, "IfTargetIsAMount" },
		{ Token::Type::Function, 315, "IfTargetIsAPlatform" },
		{ Token::Type::Function, 316, "AddStat" },
		{ Token::Type::Function, 317, "DisenchantTarget" },
		{ Token::Type::Function, 318, "DisenchantAll" },
		{ Token::Type::Function, 319, "SetVolumeNearestTeammate" },
		{ Token::Type::Function, 320, "AddShopPassage" },
		{ Token::Type::Function, 321, "TargetPayForArmor" },
		{ Token::Type::Function, 322, "JoinEvilTeam" },
		{ Token::Type::Function, 323, "JoinNullTeam" },
		{ Token::Type::Function, 324, "JoinGoodTeam" },
		{ Token::Type::Function, 325, "PitsKill" },
		{ Token::Type::Function, 326, "SetTargetToPassageID" },
		{ Token::Type::Function, 327, "MakeNameUnknown" },
		{ Token::Type::Function, 328, "SpawnExactParticleEndSpawn" },
		{ Token::Type::Function, 329, "SpawnPoofSpeedSpacingDamage" },
		{ Token::Type::Function, 330, "GiveExperienceToGoodTeam" },
		{ Token::Type::Function, 331, "DoNothing" },
		{ Token::Type::Function, 332, "GrogTarget" },
		{ Token::Type::Function, 333, "DazeTarget" },
		{ Token::Type::Function, 334, "EnableRespawn" },
		{ Token::Type::Function, 335, "DisableRespawn" },
		{ Token::Type::Function, 336, "DispelTargetEnchantID" },
		{ Token::Type::Function, 337, "IfHolderBlocked" },
		{ Token::Type::Function, 338, "GetTargetShieldProfiency" },
		{ Token::Type::Function, 339, "IfTargetHasNotFullMana" },
		{ Token::Type::Function, 340, "EnableListenSkill" },
		{ Token::Type::Function, 341, "SetTargetToLastItemUsed" },
		{ Token::Type::Function, 342, "FollowLink" },
		{ Token::Type::Function, 343, "IfOperatorIsLinux" },
		{ Token::Type::Function, 344, "IfTargetIsAWeapon" },
		{ Token::Type::Function, 345, "IfSomeoneIsStealing" },
		{ Token::Type::Function, 346, "IfTargetIsASpell" },
		{ Token::Type::Function, 347, "IfBackstabbed" },
		{ Token::Type::Function, 348, "GetTargetDamageType" },
		{ Token::Type::Function, 349, "AddTargetQuest" },
		{ Token::Type::Function, 350, "BeatQuestAllPlayers" },
		{ Token::Type::Function, 351, "IfTargetHasQuest" },
		{ Token::Type::Function, 352, "SetTargetQuestLevel" },
		{ Token::Type::Function, 353, "AddQuestAllPlayers" },
		{ Token::Type::Function, 354, "AddBlipAllEnemies" },
		{ Token::Type::Function, 355, "PitsFall" },
		{ Token::Type::Function, 356, "IfTargetIsOwner" },
		{ Token::Type::Function, 357, "SetSpeech" },
		{ Token::Type::Function, 364, "TakePicture" },
		{ Token::Type::Function, 365, "IfOperatorIsMacintosh" },
		{ Token::Type::Function, 366, "IfModuleHasIDSZ" },
		{ Token::Type::Function, 367, "MorphToTarget" },
		{ Token::Type::Function, 368, "GiveManaFlowToTarget" },
		{ Token::Type::Function, 369, "GiveManaReturnToTarget" },
		{ Token::Type::Function, 370, "SetMoney" },
		{ Token::Type::Function, 371, "IfTargetCanSeeKurses" },
		{ Token::Type::Function, 372, "SpawnAttachedCharacter" },
		{ Token::Type::Function, 373, "KurseTarget" },
		{ Token::Type::Function, 374, "SetChildContent" },
		{ Token::Type::Function, 375, "SetTargetToChild" },
		{ Token::Type::Function, 376, "SetDamageThreshold" },
		{ Token::Type::Function, 377, "AccelerateTargetUp" },
		{ Token::Type::Function, 378, "SetTargetAmmo" },
		{ Token::Type::Function, 379, "EnableInvictus" },
		{ Token::Type::Function, 380, "DisableInvictus" },
		{ Token::Type::Function, 381, "TargetDamageSelf" },
		{ Token::Type::Function, 382, "SetTargetSize" },
		{ Token::Type::Function, 383, "IfTargetIsFacingSelf" },
		{ Token::Type::Function, 384, "DrawBillboard" },
		{ Token::Type::Function, 385, "SetTargetToBlahInPassage" },
		{ Token::Type::Function, 386, "IfLevelUp" },
		{ Token::Type::Function, 387, "GiveSkillToTarget" },
		{ Token::Type::Function, 388, "SetTargetToNearbyMeleeWeapon" },
		{ Token::Type::Function, 389, "EnableStealth" },
		{ Token::Type::Function, 390, "DisableStealth" },
		{ Token::Type::Function, 391, "IfStealthed" },
		{ Token::Type::Function, 392, "SetTargetToDistantFriend" },
		{ Token::Type::Function, 393, "DisplayCharge" },
		{ Token::Type::Constant, 1, "BLAHDEAD" },
		{ Token::Type::Constant, 2, "BLAHENEMIES" },
		{ Token::Type::Constant, 4, "BLAHFRIENDS" },
		{ Token::Type::Constant, 8, "BLAHITEMS" },
		{ Token::Type::Constant, 16, "BLAHINVERTID" },
		{ Token::Type::Constant, 32, "BLAHPLAYERS" },
		{ Token::Type::Constant, 64, "BLAHSKILL" },
		{ Token::Type::Constant, 128, "BLAHQUEST" },
		{ Token::Type::Constant, 0, "STATEPARRY" },
		{ Token::Type::Constant, 1, "STATEWANDER" },
		{ Token::Type::Constant, 2, "STATEGUARD" },
		{ Token::Type::Constant, 3, "STATEFOLLOW" },
		{ Token::Type::Constant, 4, "STATESURROUND" },
		{ Token::Type::Constant, 5, "STATERETREAT" },
		{ Token::Type::Constant, 6, "STATECHARGE" },
		{ Token::Type::Constant, 7, "STATECOMBAT" },
		{ Token::Type::Constant, 4, "GRIPONLY" },
		{ Token::Type::Constant, 4, "GRIPLEFT" },
		{ Token::Type::Constant, 8, "GRIPRIGHT" },
		{ Token::Type::Constant, 0, "SPAWNORIGIN" },
		{ Token::Type::Constant, 1, "SPAWNLAST" },
		{ Token::Type::Constant, 0, "LATCHLEFT" },
		{ Token::Type::Constant, 1, "LATCHRIGHT" },
		{ Token::Type::Constant, 2, "LATCHJUMP" },
		{ Token::Type::Constant, 3, "LATCHALTLEFT" },
		{ Token::Type::Constant, 4, "LATCHALTRIGHT" },
		{ Token::Type::Constant, 5, "LATCHPACKLEFT" },
		{ Token::Type::Constant, 6, "LATCHPACKRIGHT" },
		{ Token::Type::Constant, 0, "DAMAGESLASH" },
		{ Token::Type::Constant, 1, "DAMAGECRUSH" },
		{ Token::Type::Constant, 2, "DAMAGEPOKE" },
		{ Token::Type::Constant, 3, "DAMAGEHOLY" },
		{ Token::Type::Constant, 4, "DAMAGEEVIL" },
		{ Token::Type::Constant, 5, "DAMAGEFIRE" },
		{ Token::Type::Constant, 6, "DAMAGEICE" },
		{ Token::Type::Constant, 7, "DAMAGEZAP" },
		{ Token::Type::Constant, 0, "ACTIONDA" },
		{ Token::Type::Constant, 1, "ACTIONDB" },
		{ Token::Type::Constant, 2, "ACTIONDC" },
		{ Token::Type::Constant, 3, "ACTIONDD" },
		{ Token::Type::Constant, 4, "ACTIONUA" },
		{ Token::Type::Constant, 5, "ACTIONUB" },
		{ Token::Type::Constant, 6, "ACTIONUC" },
		{ Token::Type::Constant, 7, "ACTIONUD" },
		{ Token::Type::Constant, 8, "ACTIONTA" },
		{ Token::Type::Constant, 9, "ACTIONTB" },
		{ Token::Type::Constant, 10, "ACTIONTC" },
		{ Token::Type::Constant, 11, "ACTIONTD" },
		{ Token::Type::Constant, 12, "ACTIONCA" },
		{ Token::Type::Constant, 13, "ACTIONCB" },
		{ Token::Type::Constant, 14, "ACTIONCC" },
		{ Token::Type::Constant, 15, "ACTIONCD" },
		{ Token::Type::Constant, 16, "ACTIONSA" },
		{ Token::Type::Constant, 17, "ACTIONSB" },
		{ Token::Type::Constant, 18, "ACTIONSC" },
		{ Token::Type::Constant, 19, "ACTIONSD" },
		{ Token::Type::Constant, 20, "ACTIONBA" },
		{ Token::Type::Constant, 21, "ACTIONBB" },
		{ Token::Type::Constant, 22, "ACTIONBC" },
		{ Token::Type::Constant, 23, "ACTIONBD" },
		{ Token::Type::Constant, 24, "ACTIONLA" },
		{ Token::Type::Constant, 25, "ACTIONLB" },
		{ Token::Type::Constant, 26, "ACTIONLC" },
		{ Token::Type::Constant, 27, "ACTIONLD" },
		{ Token::Type::Constant, 28, "ACTIONXA" },
		{ Token::Type::Constant, 29, "ACTIONXB" },
		{ Token::Type::Constant, 30, "ACTIONXC" },
		{ Token::Type::Constant, 31, "ACTIONXD" },
		{ Token::Type::Constant, 32, "ACTIONFA" },
		{ Token::Type::Constant, 33, "ACTIONFB" },
		{ Token::Type::Constant, 34, "ACTIONFC" },
		{ Token::Type::Constant, 35, "ACTIONFD" },
		{ Token::Type::Constant, 36, "ACTIONPA" },
		{ Token::Type::Constant, 37, "ACTIONPB" },
		{ Token::Type::Constant, 38, "ACTIONPC" },
		{ Token::Type::Constant, 39, "ACTIONPD" },
		{ Token::Type::Constant, 40, "ACTIONEA" },
		{ Token::Type::Constant, 41, "ACTIONEB" },
		{ Token::Type::Constant, 42, "ACTIONRA" },
		{ Token::Type::Constant, 43, "ACTIONZA" },
		{ Token::Type::Constant, 44, "ACTIONZB" },
		{ Token::Type::Constant, 45, "ACTIONZC" },
		{ Token::Type::Constant, 46, "ACTIONZD" },
		{ Token::Type::Constant, 47, "ACTIONWA" },
		{ Token::Type::Constant, 48, "ACTIONWB" },
		{ Token::Type::Constant, 49, "ACTIONWC" },
		{ Token::Type::Constant, 50, "ACTIONWD" },
		{ Token::Type::Constant, 51, "ACTIONJA" },
		{ Token::Type::Constant, 52, "ACTIONJB" },
		{ Token::Type::Constant, 53, "ACTIONJC" },
		{ Token::Type::Constant, 54, "ACTIONHA" },
		{ Token::Type::Constant, 55, "ACTIONHB" },
		{ Token::Type::Constant, 56, "ACTIONHC" },
		{ Token::Type::Constant, 57, "ACTIONHD" },
		{ Token::Type::Constant, 58, "ACTIONKA" },
		{ Token::Type::Constant, 59, "ACTIONKB" },
		{ Token::Type::Constant, 60, "ACTIONKC" },
		{ Token::Type::Constant, 61, "ACTIONKD" },
		{ Token::Type::Constant, 62, "ACTIONMA" },
		{ Token::Type::Constant, 63, "ACTIONMB" },
		{ Token::Type::Constant, 64, "ACTIONMC" },
		{ Token::Type::Constant, 65, "ACTIONMD" },
		{ Token::Type::Constant, 66, "ACTIONME" },
		{ Token::Type::Constant, 67, "ACTIONMF" },
		{ Token::Type::Constant, 68, "ACTIONMG" },
		{ Token::Type::Constant, 69, "ACTIONMH" },
		{ Token::Type::Constant, 70, "ACTIONMI" },
		{ Token::Type::Constant, 71, "ACTIONMJ" },
		{ Token::Type::Constant, 72, "ACTIONMK" },
		{ Token::Type::Constant, 73, "ACTIONML" },
		{ Token::Type::Constant, 74, "ACTIONMM" },
		{ Token::Type::Constant, 75, "ACTIONMN" },
		{ Token::Type::Constant, 0, "EXPSECRET" },
		{ Token::Type::Constant, 1, "EXPQUEST" },
		{ Token::Type::Constant, 2, "EXPDARE" },
		{ Token::Type::Constant, 3, "EXPKILL" },
		{ Token::Type::Constant, 4, "EXPMURDER" },
		{ Token::Type::Constant, 5, "EXPREVENGE" },
		{ Token::Type::Constant, 6, "EXPTEAMWORK" },
		{ Token::Type::Constant, 7, "EXPROLEPLAY" },
		{ Token::Type::Constant, 0, "MESSAGEDEATH" },
		{ Token::Type::Constant, 1, "MESSAGEHATE" },
		{ Token::Type::Constant, 2, "MESSAGEOUCH" },
		{ Token::Type::Constant, 3, "MESSAGEFRAG" },
		{ Token::Type::Constant, 4, "MESSAGEACCIDENT" },
		{ Token::Type::Constant, 5, "MESSAGECOSTUME" },
		{ Token::Type::Constant, 0, "ORDERMOVE" },
		{ Token::Type::Constant, 1, "ORDERATTACK" },
		{ Token::Type::Constant, 2, "ORDERASSIST" },
		{ Token::Type::Constant, 3, "ORDERSTAND" },
		{ Token::Type::Constant, 4, "ORDERTERRAIN" },
		{ Token::Type::Constant, 0, "WHITE" },
		{ Token::Type::Constant, 1, "RED" },
		{ Token::Type::Constant, 2, "YELLOW" },
		{ Token::Type::Constant, 3, "GREEN" },
		{ Token::Type::Constant, 4, "BLUE" },
		{ Token::Type::Constant, 5, "PURPLE" },
		{ Token::Type::Constant, 1, "FXNOREFLECT" },
		{ Token::Type::Constant, 2, "FXDRAWREFLECT" },
		{ Token::Type::Constant, 4, "FXANIM" },
		{ Token::Type::Constant, 8, "FXWATER" },
		{ Token::Type::Constant, 16, "FXBARRIER" },
		{ Token::Type::Constant, 32, "FXIMPASS" },
		{ Token::Type::Constant, 64, "FXDAMAGE" },
		{ Token::Type::Constant, 128, "FXSLIPPY" },
		{ Token::Type::Constant, 0, "TEAMA" },
		{ Token::Type::Constant, 1, "TEAMB" },
		{ Token::Type::Constant, 2, "TEAMC" },
		{ Token::Type::Constant, 3, "TEAMD" },
		{ Token::Type::Constant, 4, "TEAME" },
		{ Token::Type::Constant, 5, "TEAMF" },
		{ Token::Type::Constant, 6, "TEAMG" },
		{ Token::Type::Constant, 7, "TEAMH" },
		{ Token::Type::Constant, 8, "TEAMI" },
		{ Token::Type::Constant, 9, "TEAMJ" },
		{ Token::Type::Constant, 10, "TEAMK" },
		{ Token::Type::Constant, 11, "TEAML" },
		{ Token::Type::Constant, 12, "TEAMM" },
		{ Token::Type::Constant, 13, "TEAMN" },
		{ Token::Type::Constant, 14, "TEAMO" },
		{ Token::Type::Constant, 15, "TEAMP" },
		{ Token::Type::Constant, 16, "TEAMQ" },
		{ Token::Type::Constant, 17, "TEAMR" },
		{ Token::Type::Constant, 18, "TEAMS" },
		{ Token::Type::Constant, 19, "TEAMT" },
		{ Token::Type::Constant, 20, "TEAMU" },
		{ Token::Type::Constant, 21, "TEAMV" },
		{ Token::Type::Constant, 22, "TEAMW" },
		{ Token::Type::Constant, 23, "TEAMX" },
		{ Token::Type::Constant, 24, "TEAMY" },
		{ Token::Type::Constant, 25, "TEAMZ" },
		{ Token::Type::Constant, 1, "INVENTORY" },
		{ Token::Type::Constant, 2, "LEFT" },
		{ Token::Type::Constant, 3, "RIGHT" },
		{ Token::Type::Constant, 0, "EASY" },
		{ Token::Type::Constant, 1, "NORMAL" },
		{ Token::Type::Constant, 2, "HARD" },
		{ Token::Type::Variable, 0, "tmpx" },
		{ Token::Type::Variable, 1, "tmpy" },
		{ Token::Type::Variable, 2, "tmpdist" },
		{ Token::Type::Variable, 2, "tmpdistance" },
		{ Token::Type::Variable, 3, "tmpturn" },
		{ Token::Type::Variable, 4, "tmpargument" },
		{ Token::Type::Variable, 5, "rand" },
		{ Token::Type::Variable, 6, "selfx" },
		{ Token::Type::Variable, 7, "selfy" },
		{ Token::Type::Variable, 8, "selfturn" },
		{ Token::Type::Variable, 9, "selfcounter" },
		{ Token::Type::Variable, 10, "selforder" },
		{ Token::Type::Variable, 11, "selfmorale" },
		{ Token::Type::Variable, 12, "selflife" },
		{ Token::Type::Variable, 13, "targetx" },
		{ Token::Type::Variable, 14, "targety" },
		{ Token::Type::Variable, 15, "targetdistance" },
		{ Token::Type::Variable, 16, "targetturn" },
		{ Token::Type::Variable, 17, "leaderx" },
		{ Token::Type::Variable, 18, "leadery" },
		{ Token::Type::Variable, 19, "leaderdistance" },
		{ Token::Type::Variable, 20, "leaderturn" },
		{ Token::Type::Variable, 21, "gotox" },
		{ Token::Type::Variable, 22, "gotoy" },
		{ Token::Type::Variable, 23, "gotodistance" },
		{ Token::Type::Variable, 24, "targetturnto" },
		{ Token::Type::Variable, 25, "passage" },
		{ Token::Type::Variable, 26, "weight" },
		{ Token::Type::Variable, 27, "selfaltitude" },
		{ Token::Type::Variable, 28, "selfid" },
		{ Token::Type::Variable, 29, "selfhateid" },
		{ Token::Type::Variable, 30, "selfmana" },
		{ Token::Type::Variable, 31, "targetstr" },
		{ Token::Type::Variable, 32, "targetwis" },   //deprecated
		{ Token::Type::Variable, 33, "targetint" },
		{ Token::Type::Variable, 34, "targetdex" },
		{ Token::Type::Variable, 35, "targetlife" },
		{ Token::Type::Variable, 36, "targetmana" },
		{ Token::Type::Variable, 37, "targetlevel" },
		{ Token::Type::Variable, 38, "targetspeedx" },
		{ Token::Type::Variable, 39, "targetspeedy" },
		{ Token::Type::Variable, 40, "targetspeedz" },
		{ Token::Type::Variable, 41, "selfspawnx" },
		{ Token::Type::Variable, 42, "selfspawny" },
		{ Token::Type::Variable, 43, "selfstate" },
		{ Token::Type::Variable, 44, "selfstr" },
		{ Token::Type::Variable, 45, "selfwis" },
		{ Token::Type::Variable, 46, "selfint" },
		{ Token::Type::Variable, 47, "selfdex" },
		{ Token::Type::Variable, 48, "selfmanaflow" },
		{ Token::Type::Variable, 49, "targetmanaflow" },
		{ Token::Type::Variable, 50, "selfattached" },
		{ Token::Type::Variable, 51, "swingturn" },
		{ Token::Type::Variable, 52, "xydistance" },
		{ Token::Type::Variable, 53, "selfz" },
		{ Token::Type::Variable, 54, "targetaltitude" },
		{ Token::Type::Variable, 55, "targetz" },
		{ Token::Type::Variable, 56, "selfindex" },
		{ Token::Type::Variable, 57, "ownerx" },
		{ Token::Type::Variable, 58, "ownery" },
		{ Token::Type::Variable, 59, "ownerturn" },
		{ Token::Type::Variable, 60, "ownerdistance" },
		{ Token::Type::Variable, 61, "ownerturnto" },
		{ Token::Type::Variable, 62, "xyturnto" },
		{ Token::Type::Variable, 63, "selfmoney" },
		{ Token::Type::Variable, 64, "selfaccel" },
		{ Token::Type::Variable, 65, "targetexp" },
		{ Token::Type::Variable, 66, "selfammo" },
		{ Token::Type::Variable, 67, "targetammo" },
		{ Token::Type::Variable, 68, "targetmoney" },
		{ Token::Type::Variable, 69, "targetturnfrom" },
		{ Token::Type::Variable, 70, "selflevel" },
		{ Token::Type::Variable, 71, "targetreloadtime" },
		{ Token::Type::Variable, 72, "selfcontent" },
		{ Token::Type::Variable, 73, "spawndistance" },
		{ Token::Type::Variable, 74, "targetmaxlife" },
		{ Token::Type::Variable, 75, "targetteam" },
		{ Token::Type::Variable, 76, "targetarmor" },
		{ Token::Type::Variable, 77, "difficulty" },
		{ Token::Type::Variable, 78, "timehours" },
		{ Token::Type::Variable, 79, "timeminutes" },
		{ Token::Type::Variable, 80, "timeseconds" },
		{ Token::Type::Variable, 81, "datemonth" },
		{ Token::Type::Variable, 82, "dateday" },
		{ Token::Type::Operator, 0, "+" },
		{ Token::Type::Operator, 1, "-" },
		{ Token::Type::Operator, 2, "&" },
		{ Token::Type::Operator, 3, ">" },
		{ Token::Type::Operator, 4, "<" },
		{ Token::Type::Operator, 5, "*" },
		{ Token::Type::Operator, 6, "/" },
		{ Token::Type::Operator, 7, "%" },
	};

    OpList.count = 0;
    for (size_t i = 0, n = sizeof(AICODES) / sizeof(aicode_t); i < n; ++i)
    {
        strncpy(OpList.ary[OpList.count].cName, AICODES[i]._name, SDL_arraysize(OpList.ary[OpList.count].cName));
        OpList.ary[OpList.count]._type = AICODES[i]._type;
        OpList.ary[OpList.count].iValue = AICODES[i]._value;
        OpList.count++;
    }
    return true;
}

//--------------------------------------------------------------------------------------------
egolib_rv load_ai_script_vfs0(parser_state_t& ps, const std::string& loadname, ObjectProfile *ppro, script_info_t& script)
{
	ps.clear_error();
	ps._line_count = 0;
	auto fileread = std::unique_ptr<vfs_FILE,void(*)(vfs_FILE *)>
		(
			vfs_openRead(loadname.c_str()),
			[](vfs_FILE *file)
			{
				if (file) vfs_close(file);
			}
		);

	// No such file
	if (!fileread)
	{
		Log::Entry e(Log::Level::Error, __FILE__, __LINE__, __FUNCTION__);
		e << "AI script `" << loadname << "` was not found" << Log::EndOfEntry;
		Log::get() << e;
		return rv_fail;
	}

	// load the file
	size_t file_size = vfs_fileLength(fileread.get());
	if (-1 == file_size) {
		Log::Entry e(Log::Level::Error,__FILE__, __LINE__, __FUNCTION__);
		e << "unable to load AI script `" << loadname << "`" << Log::EndOfEntry;
		Log::get() << e;
		return rv_fail;
	}

	//Reset read buffer first
	ps._load_buffer.fill(CSTR_END);

	if (file_size > ps._load_buffer.size()) {
		Log::Entry e(Log::Level::Error, __FILE__, __LINE__, __FUNCTION__);
		e << "file size " << file_size << " of script file `" << loadname << "` exceeds maximum file size " << ps._load_buffer.size()
		  << Log::EndOfEntry;
		Log::get() << e;
		return rv_fail;
	}

	ps._load_buffer_count = (int)vfs_read(ps._load_buffer.data(), 1, file_size, fileread.get());
    fileread = nullptr;

	// if the file is empty, use the default script
	if (0 == ps._load_buffer_count)
	{
		Log::Entry e(Log::Level::Error, __FILE__, __LINE__, __FUNCTION__);
		e << "script file `" << loadname << "` is empty" << Log::EndOfEntry;
		Log::get() << e;
		return rv_fail;
	}

	// save the filename for error logging
	script._name = loadname;

	// we have parsed nothing yet
	script._instructions._length = 0;

	// parse/compile the scripts
	ps.parse_line_by_line(ppro, script);

	// determine the correct jumps
	parser_state_t::parse_jumps(script);

	return rv_success;
}
egolib_rv load_ai_script_vfs(parser_state_t& ps, const std::string& loadname, ObjectProfile *ppro, script_info_t& script)
{
	/// @author ZZ
	/// @details This function loads a script to memory

	if (rv_success != load_ai_script_vfs0(ps, loadname, ppro, script)) {
		Log::Entry e(Log::Level::Info, __FILE__, __LINE__, __FUNCTION__);
		e << "unable to load script file `" << loadname << "` - loading default script `" << "mp_data/script.txt" << "` instead" << Log::EndOfEntry;
		Log::get() << e;
		if (rv_success != load_ai_script_vfs0(ps, "mp_data/script.txt", ppro, script)) {
			return rv_fail;
		}
	}
	return rv_success;
}

//--------------------------------------------------------------------------------------------

void print_token(const Token& token) {
#if defined(_DEBUG)
	std::cout << token;
#endif
}

#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
void print_line( parser_state_t * ps )
{
    int i;
    char cTmp;

    printf( "\n===========\n\tfile == \"%s\"\n\tline == %d\n", globalparsename, ps->token.iLine );

    printf( "\tline == \"" );

    for ( i = 0; i < ps->line_buffer_count; i++ )
    {
        cTmp = ps->line_buffer[i];
        if ( isprint( cTmp ) )
        {
            printf( "%c", cTmp );
        }
        else
        {
            printf( "\\%03d", cTmp );
        }
    };

    printf( "\", length == %d\n", ps->line_buffer_count );
}

#endif
