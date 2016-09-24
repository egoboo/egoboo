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
#include "game/egoboo.h"
#include "egolib/Log/Entry.hpp"

namespace Log {
struct CompilerEntry : Entry {
	Id::Location _location;
	CompilerEntry(Level level, const std::string& file, int line, const std::string& function,
		          const Id::Location& location)
		: Entry(level, file, line, function), _location(location) {
		getSink() << ": " << _location.getLoadName() << ":" << _location.getLineNumber() << ": ";
	}
};
}

static bool load_ai_codes_vfs();

parser_state_t::parser_state_t()
	: _loadBuffer(1024), _token(), _linebuffer()
{
	_line_count = 0;

    load_ai_codes_vfs();
    debug_script_file = vfs_openWrite("/debug/script_debug.txt");

    _error = false;
}

parser_state_t::~parser_state_t()
{
	_line_count = 0;

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

//--------------------------------------------------------------------------------------------

std::vector<opcode_data_t> Opcodes;

bool debug_scripts = false;
vfs_FILE *debug_script_file = NULL;

//--------------------------------------------------------------------------------------------

/// Emit a token to standard output in debug mode.
void print_token(const Token& token);
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
    static void print_line();
#else
    #define print_line()
#endif

//--------------------------------------------------------------------------------------------
static bool isNewline(char x) {
    return ASCII_LINEFEED_CHAR == x || C_CARRIAGE_RETURN_CHAR == x;
}

bool parser_state_t::skipNewline(size_t& read, script_info_t& script) {
    size_t newread = read;
    if (newread < _loadBuffer.getSize()) {
        char current = _loadBuffer.get(newread);
        if (isNewline(current)) {
            newread++;
            if (newread < _loadBuffer.getSize()) {
                char old = current;
                current = _loadBuffer.get(newread);
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
    while ( read < _loadBuffer.getSize() )
    {
        if (skipNewline(read, script)) {
            _linebuffer.clear();
            return read;
        }

        cTmp = _loadBuffer.get(read);
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
    while ( read < _loadBuffer.getSize() )
    {
        cTmp = _loadBuffer.get(read);

        // we reached endline
        if (isNewline(cTmp))
        {
            break;
        }

        // we reached a comment
        if ( '/' == cTmp && '/' == _loadBuffer.get(read + 1))
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
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), _token.getLine()});
		e << "compilation error - tab character used to define spacing will cause an error `"
		  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;
    }

    // scan to the beginning of the next line
    while ( read < _loadBuffer.getSize() )
    {
        if (skipNewline(read, script)) {
            break;
        } else if (CSTR_END == _loadBuffer.get(read)) {
            read += 1;
            break;
        }

        read++;
    }

    return read;
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
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), _token.getLine()});
		e << "invalid indention - number of spaces must be even - \n"
		  << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;
        _error = true;
    }

    cnt >>= 1;
    if ( cnt > 15 )
    {
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), _token.getLine()});
		e << "invalid indention - too many spaces - \n"
	      << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
		Log::get() << e;
        _error = true;
        cnt = 15;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
void parser_state_t::parse_string(std::string string, Token& token, script_info_t& script, ObjectProfile *ppro)
{
    auto makeMessage = [&string, &token, &ppro]() {
        // Add the string as a message message to the available messages of the object.
        token.setValue(ppro->addMessage(string, true));
        token.setType(Token::Type::Constant);
        token.setIndex(MAX_OPCODE);
    };
    // The string is normal, empty string:
    if (string.length() == 0) {
        // Create a message for the string.
        makeMessage();
        // Emit a warning that the string is empty.
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), token.getLine()});
        e << "empty string literal\n" << Log::EndOfEntry;
        Log::get() << e;

        return;
    }
    // The string is not empty.
    
    // If the string begins with a shebang ...
    if (string[0] == '#') {
        // ... then it is a profile reference.
        auto fileName = string.substr(1);
        // Invalid profile as default.
        token.setValue(INVALID_PRO_REF);
        // Convert reference to slot number.
        for (const auto& element : ProfileSystem::get().getLoadedProfiles()) {
            const auto& profile = element.second;
            if (profile == nullptr) continue;
            // Is this the object we are looking for?
            if (Ego::isSuffix(profile->getPathname(), fileName)) {
                token.setValue(profile->getSlotNumber());
                break;
            }
        }

        // Do we need to load the object?
        if (!ProfileSystem::get().isValidProfileID((PRO_REF)token.getValue())) {
            auto loadName = "mp_objects/" + fileName;

            // Find first free slot number.
            for (PRO_REF ipro = MAX_IMPORT_PER_PLAYER * 4; ipro < INVALID_PRO_REF; ipro++) {
                //skip loaded profiles
                if (ProfileSystem::get().isValidProfileID(ipro)) continue;

                //found a free slot
                token.setValue(ProfileSystem::get().loadOneProfile(loadName, REF_TO_INT(ipro)));
                if (token.getValue() == ipro) break;
            }
        }

        // Failed to load object!
        if (!ProfileSystem::get().isValidProfileID((PRO_REF)token.getValue())) {
            Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), token.getLine()});
            e << "failed to load object " << token.getText() << " - \n"
                << " - \n`" << _linebuffer.data() << "`" << Log::EndOfEntry;
            Log::get() << e;
        }

        token.setType(Token::Type::Constant);
        token.setIndex(MAX_OPCODE);
    // The string is a normal, non-empty string:
    } else {
        // Create a message for the string.
        makeMessage();
    }
}
size_t parser_state_t::parse_token(Token& tok, ObjectProfile *ppro, script_info_t& script, size_t read)
{
    /// @author ZZ
    /// @details This function tells what code is being indexed by read, it
    ///    will return the next spot to read from and stick the code number
    ///    in ptok->iIndex

    char cTmp;

    Ego::Script::Buffer buffer(512);

    // Reset the token
	tok = Token();

    // Check bounds
	if ( read >= _linebuffer.size() )
    {
        return _linebuffer.size();
    }

    auto write = [&buffer](char c) {
        buffer.append(c);
    };
    auto save = [&cTmp, &write, &tok]() { write(cTmp); };
    auto next = [this, &cTmp, &read]() {
        read++;
        cTmp = _linebuffer[read];
    };
    auto writeAndNext = [&write, &next](char c) { write(c); next(); };
    auto saveAndNext = [&save, &next]() { save(); next(); };

    // Skip any initial spaces
    cTmp = _linebuffer[read];
    while (Ego::isspace(cTmp) && read < _linebuffer.size())
    {
        next();
    }

    // break if there was nothing here
    if ( read >= _linebuffer.size())
    {
        print_token(tok);
        return read;
    }

    // initialize the word
    if (C_DOUBLE_QUOTE_CHAR == cTmp) {
        // `doubleQuotedString|reference`
        // strings of the form "Here lies \"The Sandwich King\"" are not supported

        next(); // skip the leading quotation mark

        while (CSTR_END != cTmp && C_DOUBLE_QUOTE_CHAR != cTmp && read < _linebuffer.size()) {
            saveAndNext();
        }

        if (C_DOUBLE_QUOTE_CHAR == cTmp) {
            writeAndNext(CSTR_END); // skip the ending quotation mark
        } else {
            if (CSTR_END == cTmp) {
                throw Id::LexicalErrorException(__FILE__, __LINE__, {script.getName(), tok.getLine()}, "unclosed string literal");
            } else {
                throw Id::LexicalErrorException(__FILE__, __LINE__, {script.getName(), tok.getLine()}, "string literal too long");
            }
        }
        tok.setText(buffer.toString());
        parse_string(tok.getText(), tok, script, ppro);
    } else if ('+' == cTmp || '-' == cTmp || '/' == cTmp || '*' == cTmp ||
               '%' == cTmp || '>' == cTmp || '<' == cTmp || '&' == cTmp) {
        saveAndNext(); write('\0');
        int i;
        tok.setText(buffer.toString());
        for (i = 0; i < Opcodes.size(); ++i) {
            if (0 == strncmp(tok.getText().c_str(), Opcodes[i].cName, MAXCODENAMESIZE)) {
                tok.setValue(Opcodes[i].iValue);
                tok.setType(Opcodes[i]._type);
                tok.setIndex(i);
                break;
            }
        }
        // We couldn't figure out what this is, throw out an error code
        if (i == Opcodes.size()) {
            throw Id::LexicalErrorException(__FILE__, __LINE__, Id::Location(script.getName(), tok.getLine()), "not an opcode");
        }
    } else if ('=' == cTmp) {
        // `assign = '='`
        saveAndNext(); write('\0');
        tok.setText(buffer.toString());
        tok.setValue(-1);
        tok.setType(Token::Type::Operator);
        tok.setIndex(MAX_OPCODE);
    } else if ('[' == cTmp) {
        // `idsz = '[' (digit|alpha)^4 ']'`
        saveAndNext();
        for (auto i = 0; i < 4; ++i) {
            if (!Ego::isdigit(cTmp) && !Ego::isalpha(cTmp)) {
                throw std::runtime_error("invalid IDSZ");
            }
            saveAndNext();
        }
        if (cTmp != ']') {
            throw std::runtime_error("invalid IDSZ");
        }
        saveAndNext(); write('\0');
        tok.setText(buffer.toString());
        IDSZ2 idsz = IDSZ2(tok.getText());
        tok.setValue(idsz.toUint32());
        tok.setType(Token::Type::Constant);
        tok.setIndex(MAX_OPCODE);
    } else if (Ego::isdigit(cTmp)) {
        // `numericLiteral`
        do {
            saveAndNext();
        } while (Ego::isdigit(cTmp));
        write('\0');
        tok.setText(buffer.toString());
        int temporary;
        sscanf(tok.getText().c_str(), "%d", &temporary);
        tok.setValue(temporary);
        tok.setType(Token::Type::Constant);
        tok.setIndex(MAX_OPCODE);
    } else if ('_' == cTmp || Ego::isalpha(cTmp)) {
        do {
            saveAndNext();
        } while ('_' == cTmp || Ego::isdigit(cTmp) || Ego::isalpha(cTmp));
        write('\0');
        tok.setText(buffer.toString());
        int i;
        for (i = 0; i < Opcodes.size(); ++i) {
            if (0 == strncmp(tok.getText().c_str(), Opcodes[i].cName, MAXCODENAMESIZE)) {
                tok.setValue(Opcodes[i].iValue);
                tok.setType(Opcodes[i]._type);
                tok.setIndex(i);
                break;
            }
        }
        // We couldn't figure out what this is, throw out an error code
        if (i == Opcodes.size()) {
            throw Id::LexicalErrorException(__FILE__, __LINE__, {script.getName(), tok.getLine()}, "not an opcode");
        }
    } else {
        throw Id::LexicalErrorException(__FILE__, __LINE__, {script.getName(), tok.getLine()}, "unexpected symbol");
    }
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
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), tok.getLine()});
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
    for ( _token.setLine(0); read < _loadBuffer.getSize(); _token.setLine(_token.getLine() + 1) )
    {
        read = load_one_line( read, script );
        if ( 0 == _linebuffer.size() ) continue;

#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
        print_line();
#endif

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
			if ( Token::Type::Operator != _token.getType() || ( _token.getText() != "=" ) )
            {
                Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), _token.getLine()});
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
                Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), _token.getLine()});
				e << "invalid operand " << _token.getText() << " - \n"
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
                    Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, {script.getName(), _token.getLine()});
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
						               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.getText().c_str() );
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
				               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.getText().c_str() );
        }
        else if ( Token::Type::Unknown == _token.getType() )
        {
            // unknown opcode, do not process this line
			Log::get().message("%s:%d:%s: compilation error - invalid operand \"%s\"(%d) - \"%s\"\n", \
				               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.getText().c_str() );
        }
        else
        {
			Log::get().message("%s:%d:%s: compilation error - compiler is broken \"%s\"(%d) - \"%s\"\n", \
				               __FILE__, __LINE__, __FUNCTION__, script._name.c_str(), _token.getLine(), _token.getText().c_str() );
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
			index += Ego::Math::clipBits<8>( iTmp._value );
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
    #define Define(name) { Token::Type::Function, ScriptFunctions::name, #name }, 
    #define DefineAlias(alias, name) { Token::Type::Function, ScriptFunctions::alias, #alias },
    #include "egolib/Script/Functions.in"
    #undef DefineAlias
    #undef Define
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
		{ Token::Type::Variable, ScriptVariables::VARTMPX, "tmpx" },
		{ Token::Type::Variable, ScriptVariables::VARTMPY, "tmpy" },
		{ Token::Type::Variable, ScriptVariables::VARTMPDISTANCE, "tmpdist" },
		{ Token::Type::Variable, ScriptVariables::VARTMPDISTANCE, "tmpdistance" },
		{ Token::Type::Variable, ScriptVariables::VARTMPTURN, "tmpturn" },
		{ Token::Type::Variable, ScriptVariables::VARTMPARGUMENT, "tmpargument" },
		{ Token::Type::Variable, ScriptVariables::VARRAND, "rand" },
		{ Token::Type::Variable, ScriptVariables::VARSELFX, "selfx" },
		{ Token::Type::Variable, ScriptVariables::VARSELFY, "selfy" },
		{ Token::Type::Variable, ScriptVariables::VARSELFTURN, "selfturn" },
		{ Token::Type::Variable, ScriptVariables::VARSELFCOUNTER, "selfcounter" },
		{ Token::Type::Variable, ScriptVariables::VARSELFORDER, "selforder" },
		{ Token::Type::Variable, ScriptVariables::VARSELFMORALE, "selfmorale" },
		{ Token::Type::Variable, ScriptVariables::VARSELFLIFE, "selflife" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETX, "targetx" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETY, "targety" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETDISTANCE, "targetdistance" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETTURN, "targetturn" },
		{ Token::Type::Variable, ScriptVariables::VARLEADERX, "leaderx" },
		{ Token::Type::Variable, ScriptVariables::VARLEADERY, "leadery" },
		{ Token::Type::Variable, ScriptVariables::VARLEADERDISTANCE, "leaderdistance" },
		{ Token::Type::Variable, ScriptVariables::VARLEADERTURN, "leaderturn" },
		{ Token::Type::Variable, ScriptVariables::VARGOTOX, "gotox" },
		{ Token::Type::Variable, ScriptVariables::VARGOTOY, "gotoy" },
		{ Token::Type::Variable, ScriptVariables::VARGOTODISTANCE, "gotodistance" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETTURNTO, "targetturnto" },
		{ Token::Type::Variable, ScriptVariables::VARPASSAGE, "passage" },
		{ Token::Type::Variable, ScriptVariables::VARWEIGHT, "weight" },
		{ Token::Type::Variable, ScriptVariables::VARSELFALTITUDE, "selfaltitude" },
		{ Token::Type::Variable, ScriptVariables::VARSELFID, "selfid" },
		{ Token::Type::Variable, ScriptVariables::VARSELFHATEID, "selfhateid" },
		{ Token::Type::Variable, ScriptVariables::VARSELFMANA, "selfmana" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETSTR, "targetstr" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETWIS, "targetwis" },   //deprecated
		{ Token::Type::Variable, ScriptVariables::VARTARGETINT, "targetint" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETDEX, "targetdex" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETLIFE, "targetlife" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETMANA, "targetmana" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETLEVEL, "targetlevel" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETSPEEDX, "targetspeedx" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETSPEEDY, "targetspeedy" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETSPEEDZ, "targetspeedz" },
		{ Token::Type::Variable, ScriptVariables::VARSELFSPAWNX, "selfspawnx" },
		{ Token::Type::Variable, ScriptVariables::VARSELFSPAWNY, "selfspawny" },
		{ Token::Type::Variable, ScriptVariables::VARSELFSTATE, "selfstate" },
		{ Token::Type::Variable, ScriptVariables::VARSELFSTR, "selfstr" },
		{ Token::Type::Variable, ScriptVariables::VARSELFWIS, "selfwis" },
		{ Token::Type::Variable, ScriptVariables::VARSELFINT, "selfint" },
		{ Token::Type::Variable, ScriptVariables::VARSELFDEX, "selfdex" },
		{ Token::Type::Variable, ScriptVariables::VARSELFMANAFLOW, "selfmanaflow" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETMANAFLOW, "targetmanaflow" },
		{ Token::Type::Variable, ScriptVariables::VARSELFATTACHED, "selfattached" },
		{ Token::Type::Variable, ScriptVariables::VARSWINGTURN, "swingturn" },
		{ Token::Type::Variable, ScriptVariables::VARXYDISTANCE, "xydistance" },
		{ Token::Type::Variable, ScriptVariables::VARSELFZ, "selfz" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETALTITUDE, "targetaltitude" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETZ, "targetz" },
		{ Token::Type::Variable, ScriptVariables::VARSELFINDEX, "selfindex" },
		{ Token::Type::Variable, ScriptVariables::VAROWNERX, "ownerx" },
		{ Token::Type::Variable, ScriptVariables::VAROWNERY, "ownery" },
		{ Token::Type::Variable, ScriptVariables::VAROWNERTURN, "ownerturn" },
		{ Token::Type::Variable, ScriptVariables::VAROWNERDISTANCE, "ownerdistance" },
		{ Token::Type::Variable, ScriptVariables::VAROWNERTURNTO, "ownerturnto" },
		{ Token::Type::Variable, ScriptVariables::VARXYTURNTO, "xyturnto" },
		{ Token::Type::Variable, ScriptVariables::VARSELFMONEY, "selfmoney" },
		{ Token::Type::Variable, ScriptVariables::VARSELFACCEL, "selfaccel" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETEXP, "targetexp" },
		{ Token::Type::Variable, ScriptVariables::VARSELFAMMO, "selfammo" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETAMMO, "targetammo" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETMONEY, "targetmoney" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETTURNAWAY, "targetturnfrom" },
		{ Token::Type::Variable, ScriptVariables::VARSELFLEVEL, "selflevel" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETRELOADTIME, "targetreloadtime" },
		{ Token::Type::Variable, ScriptVariables::VARSELFCONTENT, "selfcontent" },
		{ Token::Type::Variable, ScriptVariables::VARSPAWNDISTANCE, "spawndistance" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETMAXLIFE, "targetmaxlife" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETTEAM, "targetteam" },
		{ Token::Type::Variable, ScriptVariables::VARTARGETARMOR, "targetarmor" },
		{ Token::Type::Variable, ScriptVariables::VARDIFFICULTY, "difficulty" },
		{ Token::Type::Variable, ScriptVariables::VARTIMEHOURS, "timehours" },
		{ Token::Type::Variable, ScriptVariables::VARTIMEMINUTES, "timeminutes" },
		{ Token::Type::Variable, ScriptVariables::VARTIMESECONDS, "timeseconds" },
		{ Token::Type::Variable, ScriptVariables::VARDATEMONTH, "datemonth" },
		{ Token::Type::Variable, ScriptVariables::VARDATEDAY, "dateday" },

		{ Token::Type::Operator, ScriptOperators::OPADD, "+" },
		{ Token::Type::Operator, ScriptOperators::OPSUB, "-" },
		{ Token::Type::Operator, ScriptOperators::OPAND, "&" },
		{ Token::Type::Operator, ScriptOperators::OPSHR, ">" },
		{ Token::Type::Operator, ScriptOperators::OPSHL, "<" },
		{ Token::Type::Operator, ScriptOperators::OPMUL, "*" },
		{ Token::Type::Operator, ScriptOperators::OPDIV, "/" },
		{ Token::Type::Operator, ScriptOperators::OPMOD, "%" },
	};

    for (size_t i = 0, n = sizeof(AICODES) / sizeof(aicode_t); i < n; ++i)
    {
        Opcodes.push_back(opcode_data_t());
        strncpy(Opcodes[i].cName, AICODES[i]._name, SDL_arraysize(Opcodes[i].cName));
        Opcodes[i]._type = AICODES[i]._type;
        Opcodes[i].iValue = AICODES[i]._value;
    }
    return true;
}

//--------------------------------------------------------------------------------------------
egolib_rv load_ai_script_vfs0(parser_state_t& ps, const std::string& loadname, ObjectProfile *ppro, script_info_t& script)
{
	ps.clear_error();
	ps._line_count = 0;
	// Clear the buffer.
    ps._loadBuffer.clear();

    // Load the entire file.
    try {
        if (!vfs_exists(loadname)) {
            return rv_fail;
        }
        vfs_readEntireFile(loadname, [&ps](size_t numberOfBytes, const char *bytes) { ps._loadBuffer.append(bytes, numberOfBytes); });
    } catch (...) {
        return rv_fail;
    }
    // Assert proper encoding: The file may not contain zero terminators.
    for (size_t i = 0; i < ps._loadBuffer.getSize(); ++i) {
        if (CSTR_END == ps._loadBuffer.get(i)) {
            return rv_fail;
        }
    }
    // Append a zero terminator.
    /** @todo This is for compatibility with legacy code. */
    ps._loadBuffer.append(CSTR_END);

    try {
        // save the filename for error logging
        script._name = loadname;

        // we have parsed nothing yet
        script._instructions._length = 0;

        // parse/compile the scripts
        ps.parse_line_by_line(ppro, script);

        // determine the correct jumps
        parser_state_t::parse_jumps(script);
    } catch (...) {
        return rv_fail;
    }

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
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
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
