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
	Location _location;
	CompilerEntry(Level level, const std::string& file, int line, const std::string& function,
		          const Location& location)
		: Entry(level, file, line, function), _location(location) {
		getSink() << ": " << _location.getLoadName() << ":" << _location.getLineNumber() << ": ";
	}
};
}

static bool load_ai_codes_vfs();

parser_state_t::parser_state_t()
	: _loadBuffer(1024), _token(), _lineBuffer(256)
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
static bool isNewline(int x) {
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
	_lineBuffer.clear();

    // try to trap all end of line conditions so we can properly count the lines
    bool tabs_warning_needed = false;
    while ( read < _loadBuffer.getSize() )
    {
        if (skipNewline(read, script)) {
            _lineBuffer.clear();
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

        _lineBuffer.append(' ');

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

            _lineBuffer.append(cTmp);
        }

        read++;
    }

    if ( !foundtext )
    {
        _lineBuffer.clear();
    }

    if ( _lineBuffer.getSize() > 0  && tabs_warning_needed )
    {
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
		e << "compilation error - tab character used to define spacing will cause an error `"
		  << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
		Log::get() << e;
    }

    // scan to the beginning of the next line
    while ( read < _loadBuffer.getSize() )
    {
        if (skipNewline(read, script)) {
            break;
        }
        read++;
    }

    return read;
}

//--------------------------------------------------------------------------------------------

line_scanner_state_t::line_scanner_state_t(Buffer *inputBuffer, const Location& location)
    : m_inputPosition(0), m_inputBuffer(inputBuffer), m_location(location),
      m_lexemeBuffer(128)
{}

Location line_scanner_state_t::getLocation() const
{
    return m_location;
}

void line_scanner_state_t::next()
{
    if (m_inputPosition < m_inputBuffer->getSize()) m_inputPosition++;
}

void line_scanner_state_t::write(int symbol)
{
    m_lexemeBuffer.append(symbol);
}

void line_scanner_state_t::save()
{
    write(getCurrent());
}

void line_scanner_state_t::saveAndNext()
{
    save();
    next();
}

int line_scanner_state_t::getCurrent() const
{
    if (m_inputPosition >= m_inputBuffer->getSize()) return EndOfInputSymbol();
    else return m_inputBuffer->get(m_inputPosition);
}

bool line_scanner_state_t::is(int symbol) const
{
    return symbol == getCurrent();
}

bool line_scanner_state_t::isDoubleQuote() const
{
    return is(DoubleQuoteSymbol());
}

bool line_scanner_state_t::isStartOfInput() const
{
    return is(StartOfInputSymbol());
}

bool line_scanner_state_t::isEndOfInput() const
{
    return is(EndOfInputSymbol());
}

bool line_scanner_state_t::isWhiteSpace() const
{
    return Ego::isspace(getCurrent());
}

bool line_scanner_state_t::isDigit() const
{
    return Ego::isdigit(getCurrent());
}

bool line_scanner_state_t::isAlphabetic() const
{
    return Ego::isalpha(getCurrent());
}

bool line_scanner_state_t::isNewLine() const
{
    return ASCII_LINEFEED_CHAR == getCurrent() || C_CARRIAGE_RETURN_CHAR == getCurrent();
}

bool line_scanner_state_t::isOperator() const
{
    return
        is('+') || is('-') ||
        is('*') || is('/') ||
        is('%') ||
        is('>') || is('<') ||
        is('&') || is('=');
}

Token line_scanner_state_t::scanWhiteSpaces()
{
    int numberOfWhiteSpaces = 0;
    if (isWhiteSpace())
    {
        do
        {
            numberOfWhiteSpaces++;
            next();
        } while (isWhiteSpace());
    }
    Token token;
    token.setLocation(getLocation());
    token.setType(Token::Type::Whitespace);
    token.setValue(numberOfWhiteSpaces);
    return token;
}

Token line_scanner_state_t::scanNewLines()
{
    int numberOfNewLines = 0;
    while (isNewLine())
    {
        // '\n' | '\r' | '\n\r' | '\r\n'
        int old = getCurrent();
        next();
        if (old != getCurrent() && isNewLine())
        {
            next();
        }
        m_location = Id::Location(m_location.getLoadName(), m_location.getLineNumber() + 1);
        numberOfNewLines++;
    }
    Token token;
    token.setLocation(getLocation());
    token.setType(Token::Type::Newline);
    token.setValue(numberOfNewLines);
    return token;
}

Token line_scanner_state_t::scanNumericLiteral()
{
    m_lexemeBuffer.clear();
    if (!isDigit())
    {
        throw RuntimeErrorException(__FILE__, __LINE__, "<internal error>");
    }
    do
    {
        saveAndNext();
    } while (isDigit());
    Token token = Token(Token::Type::NumericLiteral, getLocation());
    token.setText(m_lexemeBuffer.toString());
    return token;
}

Token line_scanner_state_t::scanName()
{
    m_lexemeBuffer.clear();
    if (!is('_') && !isAlphabetic())
    {
        throw RuntimeErrorException(__FILE__, __LINE__, "<internal error>");
    }
    do
    {
        saveAndNext();
    } while (is('_') || isDigit() || isAlphabetic());
    Token token = Token(Token::Type::Name, getLocation());
    token.setText(m_lexemeBuffer.toString());
    return token;
}

Token line_scanner_state_t::scanStringOrReference()
{
    m_lexemeBuffer.clear();
    if (!isDoubleQuote())
    {
        throw RuntimeErrorException(__FILE__, __LINE__, "<internal error>");
    }

    next(); // Skip leading quotation mark.
    // If a string starts with a shebang, its contents is considered as a reference.
    bool isReference = is('#');
    if (isReference) next();
   
    
    while (!isEndOfInput() && !isNewLine() && !isDoubleQuote())
    {
        saveAndNext();
    }

    if (isDoubleQuote())
    {
        next(); // Skip ending quotation mark.
    }
    else /* if (isNewline() || isEndOfInput()) */
    {
        throw LexicalErrorException(__FILE__, __LINE__, getLocation(), "unclosed string literal");
    }
    Token token = Token(isReference ? Token::Type::Reference : Token::Type::String, getLocation());
    token.setText(m_lexemeBuffer.toString());
    return token;
}

Token line_scanner_state_t::scanIDSZ()
{
    m_lexemeBuffer.clear();
    if (!is('['))
    {
        throw RuntimeErrorException(__FILE__, __LINE__, "<internal error>");
    }
    saveAndNext();
    for (auto i = 0; i < 4; ++i)
    {
        if (!isDigit() && !isAlphabetic())
        {
            throw LexicalErrorException(__FILE__, __LINE__, getLocation(), "invalid IDSZ");
        }
        saveAndNext();
    }
    if (!is(']'))
    {
        throw LexicalErrorException(__FILE__, __LINE__, getLocation(), "invalid IDSZ");
    }
    saveAndNext();
    Token token = Token(Token::Type::IDSZ, getLocation());
    token.setText(m_lexemeBuffer.toString());
    return token;
}

Token line_scanner_state_t::scanOperator()
{
    m_lexemeBuffer.clear();
    if (!isOperator())
    {
        throw RuntimeErrorException(__FILE__, __LINE__, "<internal error>");
    }
    //saveAndNext();
    Token token;
    switch (getCurrent())
    {
        case '+': token = Token(Token::Type::Plus, getLocation()); break;
        case '-': token = Token(Token::Type::Minus, getLocation()); break;
        case '*': token = Token(Token::Type::Multiply, getLocation()); break;
        case '/': token = Token(Token::Type::Divide, getLocation()); break;
        case '%': token = Token(Token::Type::Modulus, getLocation()); break;
        case '>': token = Token(Token::Type::ShiftRight, getLocation()); break;
        case '<': token = Token(Token::Type::ShiftLeft, getLocation()); break;
        case '&': token = Token(Token::Type::And, getLocation()); break;
        case '=': token = Token(Token::Type::Assign, getLocation()); break;
        default: throw RuntimeErrorException(__FILE__, __LINE__, "internal error");
    }
    saveAndNext();
    token.setText(m_lexemeBuffer.toString());
    return token;
}

Token parser_state_t::parse_indention(script_info_t& script, line_scanner_state_t& state)
{
    auto source = state.scanWhiteSpaces();
    if (source.getType() != Token::Type::Whitespace)
    {
        throw RuntimeErrorException(__FILE__, __LINE__, "internal error");
    }

    size_t indent = source.getValue();
    static Ego::IsOdd<int> isOdd;
    if (isOdd(indent))
    {
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
        e << "invalid indention - number of spaces must be even - \n"
          << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
        Log::get() << e;
        _error = true;
    }

    indent >>= 1;
    if (indent > 15)
    {
        Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
        e << "invalid indention - too many spaces - \n"
          << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
        Log::get() << e;
        _error = true;
        indent = 15;
    }
    Token token;
    token.setLocation(source.getLocation());
    token.setType(Token::Type::Indent);
    token.setValue(indent);
    return token;
}

//--------------------------------------------------------------------------------------------
Token parser_state_t::parse_token(ObjectProfile *ppro, script_info_t& script, line_scanner_state_t& state)
{
    /// @details This function tells what code is being indexed by read, it
    /// will return the next spot to read from and stick the code number in
    /// ptok->iIndex

    // Check bounds
    if (state.isEndOfInput())
    {
        return Token(Token::Type::EndOfLine, state.getLocation());
    }

    // Skip whitespaces.
    state.scanWhiteSpaces();

    // Stop if the line is empty.
    if (state.isEndOfInput())
    {
        return Token(Token::Type::EndOfLine, state.getLocation());
    }

    // initialize the word
    if (state.isDoubleQuote())
    {
        Token token = state.scanStringOrReference();
        if (token.getType() == Token::Type::Reference)
        {
            // If it is a profile reference.

            // Invalid profile as default.
            token.setValue(INVALID_PRO_REF);
            // Convert reference to slot number.
            for (const auto& element : ProfileSystem::get().getLoadedProfiles())
            {
                const auto& profile = element.second;
                if (profile == nullptr) continue;
                // Is this the object we are looking for?
                if (Ego::isSuffix(profile->getPathname(), token.getText()))
                {
                    token.setValue(profile->getSlotNumber());
                    break;
                }
            }

            // Do we need to load the object?
            if (!ProfileSystem::get().isValidProfileID((PRO_REF)token.getValue()))
            {
                auto loadName = "mp_objects/" + token.getText();

                // Find first free slot number.
                for (PRO_REF ipro = MAX_IMPORT_PER_PLAYER * 4; ipro < INVALID_PRO_REF; ipro++)
                {
                    //skip loaded profiles
                    if (ProfileSystem::get().isValidProfileID(ipro)) continue;

                    //found a free slot
                    token.setValue(ProfileSystem::get().loadOneProfile(loadName, REF_TO_INT(ipro)));
                    if (token.getValue() == ipro) break;
                }
            }

            // Failed to load object!
            if (!ProfileSystem::get().isValidProfileID((PRO_REF)token.getValue()))
            {
                Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, token.getLocation());
                e << "failed to load object " << token.getText() << " - \n"
                    << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
                Log::get() << e;
            }
            token.setType(Token::Type::Constant);
        }
        else
        {
            // Add the string as a message message to the available messages of the object.
            token.setValue(ppro->addMessage(token.getText(), true));
            token.setType(Token::Type::Constant);
            // Emit a warning that the string is empty.
            Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, token.getLocation());
            e << "empty string literal\n" << Log::EndOfEntry;
            Log::get() << e;
        }
        return token;
    } else if (state.isOperator()) {
        auto token = state.scanOperator();
        /// @todo Totally inconsequent "semantic" analysis.
        if (token.getText() == "=")
        {
            token.setValue(-1);
            return token;
        }
        else
        {
            auto it = std::find_if(Opcodes.cbegin(), Opcodes.cend(), [&token](const auto& opcode) { return token.getText() == opcode.cName; });
            // We couldn't figure out what this is, throw out an error code
            if (it == Opcodes.cend())
            {
                throw LexicalErrorException(__FILE__, __LINE__, state.getLocation(), "not an opcode");
            }
            token.setValue((*it).iValue);
            token.setType((*it)._type);
            token.setIndex(it - Opcodes.cbegin());
            return token;
        }
    } else if (state.is('[')) {
        Token token = state.scanIDSZ();
        token.setType(Token::Type::Constant);
        IDSZ2 idsz = IDSZ2(token.getText());
        token.setValue(idsz.toUint32());
        return token;
    } else if (state.isDigit()) {
        auto token = state.scanNumericLiteral();
        token.setType(Token::Type::Constant);
        int temporary;
        sscanf(token.getText().c_str(), "%d", &temporary);
        token.setValue(temporary);
        return token;
    } else if (state.is('_') || state.isAlphabetic()) {
        Token token = state.scanName();
        auto it = std::find_if(Opcodes.cbegin(), Opcodes.cend(), [&token](const auto& opcode) { return token.getText() == opcode.cName; });
        // We couldn't figure out what this is, throw out an error code
        if (it == Opcodes.cend())
        {
            throw LexicalErrorException(__FILE__, __LINE__, state.getLocation(), "not an opcode");
        }
        token.setValue((*it).iValue);
        token.setType((*it)._type);
        token.setIndex(it - Opcodes.cbegin());
        return token;
    } else {
        throw LexicalErrorException(__FILE__, __LINE__, state.getLocation(), "unexpected symbol");
    }
}

//--------------------------------------------------------------------------------------------
void parser_state_t::emit_opcode(const Token& token, const BIT_FIELD highbits, script_info_t& script)
{
    BIT_FIELD loc_highbits = highbits;

    // If the instruction list is full ...
    if (script._instructions.isFull())
    {
        // ... raise an exception.
        /** @todo This is not an error of the syntactical analysis. */
        throw SyntacticalErrorException(__FILE__, __LINE__, token.getLocation(), "instruction list overflow");
    }

    // Emit the opcode.
    if (Token::Type::Constant == token.getType())
    {
        loc_highbits |= Instruction::FUNCTIONBITS;
        auto constantIndex = script._instructions.getConstantPool().getOrCreateConstant(token.getValue());
        script._instructions.append(Instruction(loc_highbits | constantIndex));
    }
    else if (Token::Type::Function == token.getType())
    {
        loc_highbits |= Instruction::FUNCTIONBITS;
        script._instructions.append(Instruction(loc_highbits | token.getValue()));
    }
    else
    {
		script._instructions.append(Instruction(loc_highbits | token.getValue()));
    }
}

//--------------------------------------------------------------------------------------------

void parser_state_t::parse_line_by_line( ObjectProfile *ppro, script_info_t& script )
{
    /// @author ZF
    /// @details This parses an AI script line by line

    size_t read = 0;
    size_t line = 1;
    for (_token.setLocation({script.getName(), 1}); read < _loadBuffer.getSize(); _token.setLocation({script.getName(), _token.getLocation().getLineNumber()}))
    {
        read = load_one_line( read, script );
        if ( 0 == _lineBuffer.getSize() ) continue;

#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
        print_line();
#endif
        line_scanner_state_t state(&_lineBuffer, {script.getName(), line});

        //------------------------------
        // grab the first opcode

        _token = parse_indention(script, state);
        if (!_token.is(Token::Type::Indent))
        {
            Log::CompilerEntry e(Log::Level::Error, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
            e << "expected operator - \n"
              << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
            Log::get() << e;
            throw LexicalErrorException(__FILE__, __LINE__, _token.getLocation(), e.getText());
        }
        uint32_t highbits = SetDataBits( _token.getValue() );
        _token = parse_token(ppro, script, state);
        if ( _token.is(Token::Type::Function) )
        {
            if ( ScriptFunctions::End == _token.getValue() && 0 == highbits )
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
        else if ( _token.is(Token::Type::Variable) )
        {
            //------------------------------
            // the code type is a math operation

            int operand_index;
            int operands = 0;

            // save in the value's opcode
            emit_opcode( _token, highbits, script );

            // save a position for the operand count
            _token.setValue(0);
            operand_index = script._instructions.getNumberOfInstructions();
            emit_opcode( _token, 0, script );

            // handle the "="
            _token = parse_token(ppro, script, state);  // EQUALS
			if ( !_token.isAssignOperator() )
            {
                throw SyntacticalErrorException(__FILE__, __LINE__, _token.getLocation(), "expected an assignment operator");
            }

            //------------------------------
            // grab the next opcode

            _token = parse_token( ppro, script, state );
            if ( _token.is(Token::Type::Variable) || _token.is(Token::Type::Constant) )
            {
                // this is a value or a constant
                emit_opcode( _token, 0, script );
                operands++;

                _token = parse_token( ppro, script, state );
            }
            else if (!(_token.isOperator() && !_token.isAssignOperator()))
            {
                // this is a function or an unknown value. do not break the script.
                Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
				e << "invalid operand " << _token.getText() << " - \n"
				  << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
				Log::get() << e;

                emit_opcode( _token, 0, script );
                operands++;

                _token = parse_token( ppro, script, state );
            }

            // `((operator - assignmentOperator) value)*`
            while ( !_token.is(Token::Type::EndOfLine) )
            {
                // The current token should be a non-assignment operator.
                if ( !(_token.isOperator() && !_token.isAssignOperator()) )
                {
                    // problem with the loop
                    Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
					e << "expected operator - \n"
					  << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
					Log::get() << e;
                    break;
                }

                // the highbits are the operator's value
				highbits = SetDataBits( _token.getValue() );

                // VALUE
                _token = parse_token(ppro, script, state);
                if ( Token::Type::Constant != _token.getType() && Token::Type::Variable != _token.getType() )
                {
                    // not having a constant or a value here breaks the function. stop processsing
                    Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
                    e << "invalid operand `" << _token.getText() << "`" << Log::EndOfEntry;
                    Log::get() << e;
                    break;
                }

                emit_opcode( _token, highbits, script );
                operands++;

                // OPERATOR
                _token = parse_token( ppro, script, state);
            }
            script._instructions[operand_index].setBits(operands);
        }
        else if ( _token.is(Token::Type::Constant) )
        {
            Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
            e << "invalid constant " << _token.getText() << Log::EndOfEntry;
            Log::get() << e;
        }
        else if ( _token.is(Token::Type::Unknown) )
        {
            // unknown opcode, do not process this line
            Log::CompilerEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.getLocation());
            e << "invalid operand " << _token.getText() << Log::EndOfEntry;
            Log::get() << e;
        }
        else
        {
            throw RuntimeErrorException(__FILE__, __LINE__, "internal error");
        }

        //
        line++;
    }

    _token.setValue(ScriptFunctions::End);
    _token.setType(Token::Type::Function);
    emit_opcode( _token, 0, script );
    _token.setValue(script._instructions.getNumberOfInstructions() + 1);
    emit_opcode( _token, 0, script );
}

//--------------------------------------------------------------------------------------------
Uint32 parser_state_t::jump_goto( int index, int index_end, script_info_t& script )
{
    /// @author ZZ
    /// @details This function figures out where to jump to on a fail based on the
    ///    starting location and the following code.  The starting location
    ///    should always be a function code with indentation

    auto value = script._instructions[index];
    index += 2;
    int targetindent = GetDataBits( value.getBits() );
    int indent = 100;

    while ( indent > targetindent && index < index_end )
    {
        value = script._instructions[index];
        indent = GetDataBits ( value.getBits() );
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
                index += ( value.getBits() & 255 );
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
    uint32_t index_end = script._instructions.getNumberOfInstructions();

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
            script._instructions[index].setBits(iTmp);
            index++;
        }
        else
        {
            // Operations cover each operand
            index++;
            auto iTmp = script._instructions[index];
            index++;
			index += Ego::Math::clipBits<8>( iTmp.getBits() );
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

    #define Define(value, name) { Token::Type::Constant, value, name },
		Define(1, "BLAHDEAD")
		Define(2, "BLAHENEMIES")
		Define(4, "BLAHFRIENDS")
		Define(8, "BLAHITEMS")
		Define(16, "BLAHINVERTID")
		Define(32, "BLAHPLAYERS")
		Define(64, "BLAHSKILL")
		Define(128, "BLAHQUEST")
		Define(0, "STATEPARRY")
		Define(1, "STATEWANDER")
		Define(2, "STATEGUARD")
		Define(3, "STATEFOLLOW")
		Define(4, "STATESURROUND")
		Define(5, "STATERETREAT")
		Define(6, "STATECHARGE")
		Define(7, "STATECOMBAT")
		Define(4, "GRIPONLY")
		Define(4, "GRIPLEFT")
		Define(8, "GRIPRIGHT")
		Define(0, "SPAWNORIGIN")
		Define(1, "SPAWNLAST")
		Define(0, "LATCHLEFT")
		Define(1, "LATCHRIGHT")
		Define(2, "LATCHJUMP")
		Define(3, "LATCHALTLEFT")
		Define(4, "LATCHALTRIGHT")
		Define(5, "LATCHPACKLEFT")
		Define(6, "LATCHPACKRIGHT")
		Define(0, "DAMAGESLASH")
		Define(1, "DAMAGECRUSH")
		Define(2, "DAMAGEPOKE")
		Define(3, "DAMAGEHOLY")
		Define(4, "DAMAGEEVIL")
		Define(5, "DAMAGEFIRE")
		Define(6, "DAMAGEICE")
		Define(7, "DAMAGEZAP")
		Define(0, "ACTIONDA")
		Define(1, "ACTIONDB")
		Define(2, "ACTIONDC")
		Define(3, "ACTIONDD")
		Define(4, "ACTIONUA")
		Define(5, "ACTIONUB")
		Define(6, "ACTIONUC")
		Define(7, "ACTIONUD")
		Define(8, "ACTIONTA")
		Define(9, "ACTIONTB")
		Define(10, "ACTIONTC")
		Define(11, "ACTIONTD")
		Define(12, "ACTIONCA")
		Define(13, "ACTIONCB")
		Define(14, "ACTIONCC")
		Define(15, "ACTIONCD")
		Define(16, "ACTIONSA")
		Define(17, "ACTIONSB")
		Define(18, "ACTIONSC")
		Define(19, "ACTIONSD")
		Define(20, "ACTIONBA")
		Define(21, "ACTIONBB")
		Define(22, "ACTIONBC")
		Define(23, "ACTIONBD")
		Define(24, "ACTIONLA")
		Define(25, "ACTIONLB")
		Define(26, "ACTIONLC")
		Define(27, "ACTIONLD")
		Define(28, "ACTIONXA")
		Define(29, "ACTIONXB")
		Define(30, "ACTIONXC")
		Define(31, "ACTIONXD")
		Define(32, "ACTIONFA")
		Define(33, "ACTIONFB")
		Define(34, "ACTIONFC")
		Define(35, "ACTIONFD")
		Define(36, "ACTIONPA")
		Define(37, "ACTIONPB")
		Define(38, "ACTIONPC")
		Define(39, "ACTIONPD")
		Define(40, "ACTIONEA")
		Define(41, "ACTIONEB")
		Define(42, "ACTIONRA")
		Define(43, "ACTIONZA")
		Define(44, "ACTIONZB")
		Define(45, "ACTIONZC")
		Define(46, "ACTIONZD")
		Define(47, "ACTIONWA")
		Define(48, "ACTIONWB")
		Define(49, "ACTIONWC")
		Define(50, "ACTIONWD")
		Define(51, "ACTIONJA")
		Define(52, "ACTIONJB")
		Define(53, "ACTIONJC")
		Define(54, "ACTIONHA")
		Define(55, "ACTIONHB")
		Define(56, "ACTIONHC")
		Define(57, "ACTIONHD")
		Define(58, "ACTIONKA")
		Define(59, "ACTIONKB")
		Define(60, "ACTIONKC")
		Define(61, "ACTIONKD")
		Define(62, "ACTIONMA")
		Define(63, "ACTIONMB")
		Define(64, "ACTIONMC")
		Define(65, "ACTIONMD")
		Define(66, "ACTIONME")
		Define(67, "ACTIONMF")
		Define(68, "ACTIONMG")
		Define(69, "ACTIONMH")
		Define(70, "ACTIONMI")
		Define(71, "ACTIONMJ")
		Define(72, "ACTIONMK")
		Define(73, "ACTIONML")
		Define(74, "ACTIONMM")
		Define(75, "ACTIONMN")
		Define(0, "EXPSECRET")
		Define(1, "EXPQUEST")
		Define(2, "EXPDARE")
		Define(3, "EXPKILL")
		Define(4, "EXPMURDER")
		Define(5, "EXPREVENGE")
		Define(6, "EXPTEAMWORK")
		Define(7, "EXPROLEPLAY")
		Define(0, "MESSAGEDEATH")
		Define(1, "MESSAGEHATE")
		Define(2, "MESSAGEOUCH")
		Define(3, "MESSAGEFRAG")
		Define(4, "MESSAGEACCIDENT")
		Define(5, "MESSAGECOSTUME")
		Define(0, "ORDERMOVE")
		Define(1, "ORDERATTACK")
		Define(2, "ORDERASSIST")
		Define(3, "ORDERSTAND")
		Define(4, "ORDERTERRAIN")
		Define(0, "WHITE")
		Define(1, "RED")
		Define(2, "YELLOW")
		Define(3, "GREEN")
		Define(4, "BLUE")
		Define(5, "PURPLE")
		Define(1, "FXNOREFLECT")
		Define(2, "FXDRAWREFLECT")
		Define(4, "FXANIM")
		Define(8, "FXWATER")
		Define(16, "FXBARRIER")
		Define(32, "FXIMPASS")
		Define(64, "FXDAMAGE")
		Define(128, "FXSLIPPY")
		Define(0, "TEAMA")
		Define(1, "TEAMB")
		Define(2, "TEAMC")
		Define(3, "TEAMD")
		Define(4, "TEAME")
		Define(5, "TEAMF")
		Define(6, "TEAMG")
		Define(7, "TEAMH")
		Define(8, "TEAMI")
		Define(9, "TEAMJ")
		Define(10, "TEAMK")
		Define(11, "TEAML")
		Define(12, "TEAMM")
		Define(13, "TEAMN")
		Define(14, "TEAMO")
		Define(15, "TEAMP")
		Define(16, "TEAMQ")
		Define(17, "TEAMR")
		Define(18, "TEAMS")
		Define(19, "TEAMT")
		Define(20, "TEAMU")
		Define(21, "TEAMV")
		Define(22, "TEAMW")
		Define(23, "TEAMX")
		Define(24, "TEAMY")
		Define(25, "TEAMZ")
		Define(1, "INVENTORY")
		Define(2, "LEFT")
		Define(3, "RIGHT")
		Define(0, "EASY")
		Define(1, "NORMAL")
        Define(2, "HARD")
    #undef Define

    #define Define(cName, eName) { Token::Type::Variable, ScriptVariables::cName, eName },
    #define DefineAlias(cName, eName) { Token::Type::Variable, ScriptVariables::cName, eName },
    #include "egolib/Script/Variables.in"
    #undef DefineAlias
    #undef Define

		{ Token::Type::Plus, ScriptOperators::OPADD, "+" },
		{ Token::Type::Minus, ScriptOperators::OPSUB, "-" },
		{ Token::Type::And, ScriptOperators::OPAND, "&" },
		{ Token::Type::ShiftRight, ScriptOperators::OPSHR, ">" },
		{ Token::Type::ShiftLeft, ScriptOperators::OPSHL, "<" },
		{ Token::Type::Multiply, ScriptOperators::OPMUL, "*" },
		{ Token::Type::Divide, ScriptOperators::OPDIV, "/" },
		{ Token::Type::Modulus, ScriptOperators::OPMOD, "%" },
	};

    for (size_t i = 0, n = sizeof(AICODES) / sizeof(aicode_t); i < n; ++i)
    {
        Opcodes.push_back(opcode_data_t());
        Opcodes[i].cName = AICODES[i]._name;
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
    try {
        // save the filename for error logging
        script._name = loadname;

        // we have parsed nothing yet
        script._instructions.clear();

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
