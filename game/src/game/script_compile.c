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
#include "egolib/Script/CLogEntry.hpp"

static bool load_ai_codes_vfs();

parser_state_t::parser_state_t()
	: _loadBuffer(), _token(), _lineBuffer()
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
void print_token(const Ego::Script::PDLToken& token);
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

        if (!inside_string && Ego::iscntrl(cTmp))
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
        Ego::Script::CLogEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.get_start_location());
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

line_scanner_state_t::line_scanner_state_t(Ego::Script::Buffer *inputBuffer, const id::location& location)
    : m_token(Ego::Script::PDLTokenKind::StartOfLine, location, location), m_inputPosition(0),
      m_inputBuffer(inputBuffer), m_location(location), m_lexemeBuffer()
{}

id::location line_scanner_state_t::getLocation() const
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

void line_scanner_state_t::writeAndNext(int symbol)
{
    write(symbol);
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

bool line_scanner_state_t::isOneOf(int symbol1, int symbol2) const
{
    return is(symbol1)
        || is(symbol2);
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
    return is(LinefeedSymbol())
        || is(CarriageReturnSymbol());
}

bool line_scanner_state_t::isOperator() const
{
    return isOneOf('+', '-', 
                   '*', '/',
                   '%',
                   '>', '<',
                   '&', '=');
}

bool line_scanner_state_t::isControl() const
{
    return Ego::iscntrl(getCurrent());
}

void line_scanner_state_t::emit(const Ego::Script::PDLToken& token)
{
    m_token = token;
}

void line_scanner_state_t::emit(Ego::Script::PDLTokenKind kind, const id::location& start, const id::location& end)
{
    m_token = Ego::Script::PDLToken(kind, start, end);
}

void line_scanner_state_t::emit(Ego::Script::PDLTokenKind kind, const id::location& start, const id::location& end,
                                const std::string& lexeme)
{
    m_token = Ego::Script::PDLToken(kind, start, end, lexeme);
}

void line_scanner_state_t::emit(Ego::Script::PDLTokenKind kind, const id::location& start, const id::location& end,
                                int value)
{
    m_token = Ego::Script::PDLToken(kind, start, end);
    m_token.setValue(value);
}

Ego::Script::PDLToken line_scanner_state_t::scanWhiteSpaces()
{
    auto startLocation = getLocation();
    int numberOfWhiteSpaces = 0;
    if (isWhiteSpace())
    {
        do
        {
            if (is(TabulatorSymbol()))
            {
                Ego::Script::CLogEntry e(Log::Level::Warning, __FILE__, __LINE__, __FUNCTION__, getLocation());
                e << "tabulator character in source file" << Log::EndOfEntry;
                Log::get() << e;
            }
            numberOfWhiteSpaces++;
            next();
        } while (isWhiteSpace());
    }
    auto endLocation = getLocation();
    emit(Ego::Script::PDLTokenKind::Whitespace, startLocation, endLocation, numberOfWhiteSpaces);
    return m_token;
}

Ego::Script::PDLToken line_scanner_state_t::scanNewLines()
{
    auto startLocation = getLocation();
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
        m_location = id::location(m_location.file_name(), m_location.line_number() + 1);
        numberOfNewLines++;
    }
    auto endLocation = getLocation();
    emit(Ego::Script::PDLTokenKind::Newline, startLocation, endLocation, numberOfNewLines);
    return m_token;
}

Ego::Script::PDLToken line_scanner_state_t::scanNumericLiteral()
{
    m_lexemeBuffer.clear();
    auto startLocation = getLocation();
    if (!isDigit())
    {
        throw id::runtime_error(__FILE__, __LINE__, "<internal error>");
    }
    do
    {
        saveAndNext();
    } while (isDigit());
    auto endLocation = getLocation();
    emit(Ego::Script::PDLTokenKind::NumberLiteral, startLocation,
         endLocation, m_lexemeBuffer.toString());
    return m_token;
}

Ego::Script::PDLToken line_scanner_state_t::scanName()
{
    m_lexemeBuffer.clear();
    auto startLocation = getLocation();
    if (!is('_') && !isAlphabetic())
    {
        throw id::runtime_error(__FILE__, __LINE__, "<internal error>");
    }
    do
    {
        saveAndNext();
    } while (is('_') || isDigit() || isAlphabetic());
    auto endLocation = getLocation();
    emit(Ego::Script::PDLTokenKind::Name, startLocation, endLocation,
         m_lexemeBuffer.toString());
    return m_token;
}

Ego::Script::PDLToken line_scanner_state_t::scanStringOrReference()
{
    m_lexemeBuffer.clear();
    auto startLocation = getLocation();
    if (!isDoubleQuote())
    {
        throw id::runtime_error(__FILE__, __LINE__, "<internal error>");
    }

    next(); // Skip leading quotation mark.
    // If a string starts with a shebang, its contents is considered as a reference.
    bool isReference = is('#');
    if (isReference) next();
   
    
    while (!isEndOfInput() && !isNewLine() && !isDoubleQuote())
    {
        if (is(TabulatorSymbol()))
        {
            // convert tab characters to the '~' symbol.
            writeAndNext(TildeSymbol());
        }
        else if (isWhiteSpace() || isControl())
        {
            // whitespace symbols and control symbols are converted to the '_' symbol.
            writeAndNext(UnderscoreSymbol());
        }
        else
        {
            saveAndNext();
        }
    }

    if (isDoubleQuote())
    {
        next(); // Skip ending quotation mark.
    }
    else /* if (isNewline() || isEndOfInput()) */
    {
        throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, getLocation(), "unclosed string literal");
    }
    auto endLocation = getLocation();
    emit(isReference ? Ego::Script::PDLTokenKind::ReferenceLiteral : Ego::Script::PDLTokenKind::StringLiteral,
         startLocation, endLocation, m_lexemeBuffer.toString());
    return m_token;
}

Ego::Script::PDLToken line_scanner_state_t::scanIDSZ()
{
    m_lexemeBuffer.clear();
    auto startLocation = getLocation();
    if (!is('['))
    {
        throw id::runtime_error(__FILE__, __LINE__, "<internal error>");
    }
    saveAndNext();
    for (auto i = 0; i < 4; ++i)
    {
        if (!isDigit() && !isAlphabetic())
        {
            throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, getLocation(), "invalid IDSZ");
        }
        saveAndNext();
    }
    if (!is(']'))
    {
        throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, getLocation(), "invalid IDSZ");
    }
    saveAndNext();
    auto endLocation = getLocation();
    emit(Ego::Script::PDLTokenKind::IdszLiteral, startLocation,
         endLocation, m_lexemeBuffer.toString());
    return m_token;
}

Ego::Script::PDLToken line_scanner_state_t::scanOperator()
{
    m_lexemeBuffer.clear();
    auto startLocation = getLocation();
    if (!isOperator())
    {
        throw id::runtime_error(__FILE__, __LINE__, "<internal error>");
    }
    auto current = getCurrent();
    saveAndNext();
    auto endLocation = getLocation();
    switch (current)
    {
        case '+':
            emit(Ego::Script::PDLTokenKind::Plus, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '-':
            emit(Ego::Script::PDLTokenKind::Minus, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '*':
            emit(Ego::Script::PDLTokenKind::Multiply, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '/':
            emit(Ego::Script::PDLTokenKind::Divide, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '%':
            emit(Ego::Script::PDLTokenKind::Modulus, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '>':
            emit(Ego::Script::PDLTokenKind::ShiftRight, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '<':
            emit(Ego::Script::PDLTokenKind::ShiftLeft, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '&':
            emit(Ego::Script::PDLTokenKind::And, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        case '=':
            emit(Ego::Script::PDLTokenKind::Assign, startLocation, endLocation,
                 m_lexemeBuffer.toString());
            break;
        default:
            throw id::runtime_error(__FILE__, __LINE__, "internal error");
    }
    return m_token;
}

Ego::Script::PDLToken parser_state_t::parse_indention(script_info_t& script, line_scanner_state_t& state)
{
    auto source = state.scanWhiteSpaces();
    if (source.category() != Ego::Script::PDLTokenKind::Whitespace)
    {
        throw id::runtime_error(__FILE__, __LINE__, "internal error");
    }

    size_t indent = source.getValue();
    if (id::is_odd(indent))
    {
        Ego::Script::CLogEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.get_start_location());
        e << "invalid indention - number of spaces must be even - \n"
          << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
        Log::get() << e;
        _error = true;
    }

    indent >>= 1;
    if (indent > 15)
    {
        Ego::Script::CLogEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, _token.get_start_location());
        e << "invalid indention - too many spaces - \n"
          << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
        Log::get() << e;
        _error = true;
        indent = 15;
    }
    auto token = Ego::Script::PDLToken(Ego::Script::PDLTokenKind::Indent, source.get_start_location(), source.getEndLocation());
    token.setValue(indent);
    return token;
}

//--------------------------------------------------------------------------------------------
Ego::Script::PDLToken parser_state_t::parse_token(ObjectProfile *ppro, script_info_t& script, line_scanner_state_t& state)
{
    /// @details This function tells what code is being indexed by read, it
    /// will return the next spot to read from and stick the code number in
    /// ptok->iIndex

    // Check bounds
    if (state.isEndOfInput())
    {
        return Ego::Script::PDLToken(Ego::Script::PDLTokenKind::EndOfLine, state.getLocation(), state.getLocation());
    }

    // Skip whitespaces.
    state.scanWhiteSpaces();

    // Stop if the line is empty.
    if (state.isEndOfInput())
    {
        return Ego::Script::PDLToken(Ego::Script::PDLTokenKind::EndOfLine, state.getLocation(), state.getLocation());
    }

    // initialize the word
    if (state.isDoubleQuote())
    {
        auto token = state.scanStringOrReference();
        if (token.category() == Ego::Script::PDLTokenKind::ReferenceLiteral)
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
                if (id::is_suffix(profile->getPathname(), token.get_lexeme()))
                {
                    token.setValue(profile->getSlotNumber().get());
                    break;
                }
            }

            // Do we need to load the object?
            if (!ProfileSystem::get().isLoaded((PRO_REF)token.getValue()))
            {
                auto loadName = "mp_objects/" + token.get_lexeme();

                // Find first free slot number.
                for (PRO_REF ipro = MAX_IMPORT_PER_PLAYER * 4; ipro < INVALID_PRO_REF; ipro++)
                {
                    //skip loaded profiles
                    if (ProfileSystem::get().isLoaded(ipro)) continue;

                    //found a free slot
                    token.setValue(ProfileSystem::get().loadOneProfile(loadName, REF_TO_INT(ipro)).get());
                    if (token.getValue() == ipro) break;
                }
            }

            // Failed to load object!
            if (!ProfileSystem::get().isLoaded((PRO_REF)token.getValue()))
            {
                Ego::Script::CLogEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, token.get_start_location());
                e << "failed to load object " << token.get_lexeme() << " - \n"
                    << " - \n`" << _lineBuffer.toString() << "`" << Log::EndOfEntry;
                Log::get() << e;
            }
            token.category(Ego::Script::PDLTokenKind::Constant);
        }
        else
        {
            // Add the string as a message message to the available messages of the object.
            token.setValue(ppro->addMessage(token.get_lexeme(), true));
            token.category(Ego::Script::PDLTokenKind::Constant);
            // Emit a warning that the string is empty.
            Ego::Script::CLogEntry e(Log::Level::Message, __FILE__, __LINE__, __FUNCTION__, token.get_start_location());
            e << "empty string literal\n" << Log::EndOfEntry;
            Log::get() << e;
        }
        return token;
    } else if (state.isOperator()) {
        auto token = state.scanOperator();
        switch (token.category())
        {
            case Ego::Script::PDLTokenKind::Plus: token.setValue(Ego::Script::ScriptOperators::OPADD); break;
            case Ego::Script::PDLTokenKind::Minus: token.setValue(Ego::Script::ScriptOperators::OPSUB); break;
            case Ego::Script::PDLTokenKind::And: token.setValue(Ego::Script::ScriptOperators::OPAND); break;
            case Ego::Script::PDLTokenKind::ShiftRight: token.setValue(Ego::Script::ScriptOperators::OPSHR); break;
            case Ego::Script::PDLTokenKind::ShiftLeft: token.setValue(Ego::Script::ScriptOperators::OPSHL); break;
            case Ego::Script::PDLTokenKind::Multiply: token.setValue(Ego::Script::ScriptOperators::OPMUL); break;
            case Ego::Script::PDLTokenKind::Divide: token.setValue(Ego::Script::ScriptOperators::OPDIV); break;
            case Ego::Script::PDLTokenKind::Modulus: token.setValue(Ego::Script::ScriptOperators::OPMOD); break;
            case Ego::Script::PDLTokenKind::Assign: token.setValue(-1); break;
            default: throw id::runtime_error(__FILE__, __LINE__, "internal error");
        };
        return token;
    } else if (state.is('[')) {
        auto token = state.scanIDSZ();
        token.category(Ego::Script::PDLTokenKind::Constant);
        IDSZ2 idsz = IDSZ2(token.get_lexeme());
        token.setValue(idsz.toUint32());
        return token;
    } else if (state.isDigit()) {
        auto token = state.scanNumericLiteral();
        token.category(Ego::Script::PDLTokenKind::Constant);
        int temporary;
        sscanf(token.get_lexeme().c_str(), "%d", &temporary);
        token.setValue(temporary);
        return token;
    } else if (state.is('_') || state.isAlphabetic()) {
        auto token = state.scanName();
        auto it = std::find_if(Opcodes.cbegin(), Opcodes.cend(), [&token](const auto& opcode) { return token.get_lexeme() == opcode.cName; });
        // We couldn't figure out what this is, throw out an error code
        if (it == Opcodes.cend())
        {
            throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, state.getLocation(), "not an opcode");
        }
        token.setValue((*it).iValue);
        token.category((*it)._kind);
        return token;
    } else {
        throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, state.getLocation(), "unexpected symbol");
    }
}

//--------------------------------------------------------------------------------------------
void parser_state_t::emit_opcode(const Ego::Script::PDLToken& token, const BIT_FIELD highbits, script_info_t& script)
{
    BIT_FIELD loc_highbits = highbits;

    // Emit the opcode.
    if (Ego::Script::PDLTokenKind::Constant == token.category())
    {
        loc_highbits |= Instruction::FUNCTIONBITS;
        auto constantIndex = script._instructions.getConstantPool().getOrCreateConstant(token.getValue());
        script._instructions.append(Instruction(loc_highbits | constantIndex));
    }
    else if (Ego::Script::PDLTokenKind::Function == token.category())
    {
        loc_highbits |= Instruction::FUNCTIONBITS;
        auto constantIndex = script._instructions.getConstantPool().getOrCreateConstant(token.getValue());
        script._instructions.append(Instruction(loc_highbits | constantIndex));
    }
    else if (Ego::Script::PDLTokenKind::Variable == token.category())
    {
        auto constantIndex = script._instructions.getConstantPool().getOrCreateConstant(token.getValue());
        script._instructions.append(Instruction(loc_highbits | constantIndex));
    }
    else
    {
        /** @todo This is not an error of the syntactical analysis. */
        throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::syntactical, token.get_start_location(), "unsupported token");
    }
}

//--------------------------------------------------------------------------------------------

void parser_state_t::raise(bool raiseException, Log::Level level, const Ego::Script::PDLToken& received, const std::vector<Ego::Script::PDLTokenKind>& expected)
{
    Ego::Script::CLogEntry e(level, __FILE__, __LINE__, __FUNCTION__, received.get_start_location());
    if (expected.size() > 0)
    {
        e << "expected ";
        size_t index = 0;
        e << "`" << toString(expected[index]) << "`";
        index++;
        if (expected.size() == 2)
        {
            e << " or" << "`" << toString(expected[index]) << "`";
        }
        else if (expected.size() > 2)
        {
            for (; index < expected.size() - 1; index++)
            {
                e << ", " << "`" << toString(expected[index]) << "`";
            }
            e << ", or" << "`" << toString(expected[index]) << "`";
        }
        e << ", ";
    }
    e << "received " << "`" << toString(received.category()) << "`" << Log::EndOfEntry;
    Log::get() << e;
    if (raiseException)
    {
        throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::syntactical, received.get_start_location(), e.getText());
    }
}

void parser_state_t::parse_line_by_line( ObjectProfile *ppro, script_info_t& script )
{
    /// @author ZF
    /// @details This parses an AI script line by line

    size_t read = 0;
    size_t line = 1;
    for (_token.set_start_location({script.getName(), 1}); read < _loadBuffer.getSize(); _token.set_start_location({script.getName(), _token.get_start_location().line_number()}))
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
        if (!_token.is_one_of(Ego::Script::PDLTokenKind::Indent))
        {
            this->raise(true, Log::Level::Error, _token, {Ego::Script::PDLTokenKind::Indent});
        }
        auto indent = _token.getValue();
        _token = parse_token(ppro, script, state);

        /* `function` */
        if ( _token.is_one_of(Ego::Script::PDLTokenKind::Function) )
        {
            if ( Ego::Script::ScriptFunctions::End == _token.getValue() && 0 == indent )
            {
                // Stop processing.
                break;
            }

            // Emit the instruction.
            emit_opcode( _token, SetDataBits(indent), script );

            // Space for the control(?) code.
            _token.setValue(0);
            emit_opcode( _token, 0, script );

        }
        /* `variable assignOperator expression` */
        else if ( _token.is_one_of(Ego::Script::PDLTokenKind::Variable) )
        {
            int operand_index;
            int operands = 0;

            // Emit the opcode and save a position for the operand count.
            emit_opcode( _token, SetDataBits(indent), script );
            _token.setValue(0);
            operand_index = script._instructions.getNumberOfInstructions();
            emit_opcode( _token, 0, script );
            _token = parse_token(ppro, script, state);

            // `assignOperator`
			if ( !_token.isAssignOperator() )
            {
                this->raise(true, Log::Level::Error, _token, {Ego::Script::PDLTokenKind::Assign});
            }
            _token = parse_token(ppro, script, state);

            auto isOperator = [](const Ego::Script::PDLToken& token)
            {
                return token.isOperator() && !token.isAssignOperator();
            };

            auto isOperand = [](const Ego::Script::PDLToken& token)
            {
                return token.is_one_of(Ego::Script::PDLTokenKind::Variable,
                                       Ego::Script::PDLTokenKind::Constant);
            };
            
            if (isOperand(_token))
            {
                emit_opcode(_token, 0, script);
                operands++;
                _token = parse_token(ppro, script, state);
            }
            // `unaryPlus|unaryMinus`
            else if (_token.is_one_of(Ego::Script::PDLTokenKind::Plus, Ego::Script::PDLTokenKind::Minus))
            {
                // We have some expression of the format `('+'|'-') ...` and transform this into
                // `0 ('+'|'-') ...`. This is as close as we can currently get to proper code.
                auto token = Ego::Script::PDLToken(Ego::Script::PDLTokenKind::Constant, _token.get_start_location(), _token.getEndLocation(), "0");
                token.setValue(0);
                emit_opcode(token, 0, script);
                operands++;
                // Do not proceed.
            }
            else
            {
                this->raise(true, Log::Level::Error, _token, {Ego::Script::PDLTokenKind::Plus, Ego::Script::PDLTokenKind::Minus,
                            Ego::Script::PDLTokenKind::Variable, Ego::Script::PDLTokenKind::Constant});
            }

            // `((operator - assignOperator) (constant|variable))*`
            while ( !_token.is_one_of(Ego::Script::PDLTokenKind::EndOfLine) )
            {
                // `operator`
                if (!isOperator(_token))
                {
                    this->raise(true, Log::Level::Warning, _token,
                                {Ego::Script::PDLTokenKind::Plus, Ego::Script::PDLTokenKind::Minus,
                                 Ego::Script::PDLTokenKind::Multiply, Ego::Script::PDLTokenKind::Divide,
                                 Ego::Script::PDLTokenKind::Modulus,
                                 Ego::Script::PDLTokenKind::And,
                                 Ego::Script::PDLTokenKind::ShiftLeft, Ego::Script::PDLTokenKind::ShiftRight});
                }

				auto opcode = _token.getValue(); // The opcode.

                // `(constant|variable)`
                _token = parse_token(ppro, script, state);
                if (!isOperand(_token))
                {
                    this->raise(true, Log::Level::Error, _token, {Ego::Script::PDLTokenKind::Constant, Ego::Script::PDLTokenKind::Variable});
                }

                emit_opcode( _token, SetDataBits(opcode), script );
                operands++;
                
                _token = parse_token( ppro, script, state);
            }
            script._instructions[operand_index].setBits(operands);
        }
        else
        {
            this->raise(true, Log::Level::Error, _token, {Ego::Script::PDLTokenKind::Function, Ego::Script::PDLTokenKind::Variable});
        }

        //
        line++;
    }

    _token.setValue(Ego::Script::ScriptFunctions::End);
    _token.category(Ego::Script::PDLTokenKind::Function);
    emit_opcode( _token, 0, script );
    _token.setValue(script._instructions.getNumberOfInstructions() + 1);
    emit_opcode( _token, 0, script );
}

//--------------------------------------------------------------------------------------------
uint32_t parser_state_t::jump_goto( int index, int index_end, script_info_t& script )
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
		/// The kind.
		Ego::Script::PDLTokenKind _kind;
		/// The value of th constant.
		uint32_t _value;
		/// The name.
		const char *_name;
	};

	static const aicode_t AICODES[] =
	{
    #define Define(name) { Ego::Script::PDLTokenKind::Function, Ego::Script::ScriptFunctions::name, #name }, 
    #define DefineAlias(alias, name) { Ego::Script::PDLTokenKind::Function, Ego::Script::ScriptFunctions::alias, #alias },
    #include "egolib/Script/Functions.in"
    #undef DefineAlias
    #undef Define

    #define Define(value, name) { Ego::Script::PDLTokenKind::Constant, value, name },
    #include "egolib/Script/Constants.in"
    #undef Define

    #define Define(cName, eName) { Ego::Script::PDLTokenKind::Variable, Ego::Script::ScriptVariables::cName, eName },
    #define DefineAlias(cName, eName) { Ego::Script::PDLTokenKind::Variable, Ego::Script::ScriptVariables::cName, eName },
    #include "egolib/Script/Variables.in"
    #undef DefineAlias
    #undef Define
	};

    for (size_t i = 0, n = sizeof(AICODES) / sizeof(aicode_t); i < n; ++i)
    {
        Opcodes.push_back(opcode_data_t());
        Opcodes[i].cName = AICODES[i]._name;
        Opcodes[i]._kind = AICODES[i]._kind;
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

void print_token(const Ego::Script::PDLToken& token) {
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
