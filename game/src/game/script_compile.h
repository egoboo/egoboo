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

/// @file game/script_compile.h

#pragma once

#include "game/script_scanner.hpp"
#include "game/egoboo.h"
#include "egolib/Script/script.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// AI stuff
#define MAXLINESIZE         1024
#define MAX_OPCODE          1024                ///< Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64

#define END_VALUE    (script_t::Instruction::FUNCTIONBIT | FEND)

inline int GetDataBits(int x) { return (x >> 27) & 0x0F; }
inline int SetDataBits(int x) { return (x & 0x0F ) << 27; }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool debug_scripts;
extern vfs_FILE * debug_script_file;

/// temporary data describing a single egoscript opcode
struct opcode_data_t
{
    Token::Type _type;
    uint32_t iValue;
    char cName[MAXCODENAMESIZE];
};

/// The opcodes.
extern std::vector<opcode_data_t> Opcodes;

//--------------------------------------------------------------------------------------------

template <size_t Capacity>
struct buffer_t {
    size_t _size;
    char _elements[Capacity];
    buffer_t() {
        _size = 0;
        _elements[0] = CSTR_END;
    }
    size_t capacity() const {
        return Capacity;
    }
    char& operator[](size_t i) {
        return _elements[i];
    }
    size_t size() const {
        return _size;
    }
    void clear() {
        _elements[0] = CSTR_END;
        _size = 0;
    }
    void append(char c) {
        if (_size >= Capacity) {
            throw std::runtime_error("buffer overflow");
        }
        _elements[_size++] = c;
        _elements[_size] = CSTR_END;
    }
    const char *data() const {
        return _elements;
    }
    // 0 (before the first character) and n-1 (before the last character), n (behind the last character)
    void insert(char c, size_t i) {
        if (i > _size) {
            throw std::runtime_error("index out of bounds");
        }
        if (size() >= capacity()) {
            throw std::runtime_error("buffer overflow");
        }
        size_t j = i;
        char d = c;
        // Bubble the values starting at i (inclusive) up.
        while (j <= _size)
        {
            std::swap(_elements[j], d); // Swap the existing value with the incoming value.
            j++;
        }
        _elements[++_size] = CSTR_END;

    }
};

// the current state of the parser
struct parser_state_t : public Ego::Core::Singleton<parser_state_t>
{
protected:
    friend Ego::Core::Singleton<parser_state_t>::CreateFunctorType;
    friend Ego::Core::Singleton<parser_state_t>::DestroyFunctorType;
	/// @brief Construct this parser.
	/// @remark Intentionally protected.
	parser_state_t();
	/// @brief Destruct this parser.
    /// @remark Intentionally protected.
	virtual ~parser_state_t();
public:
    bool _error;
    Token _token;
    int _line_count;

protected:
    // @brief Skip '\n', '\r', '\n\r' or '\r\n'.
    // @return @a true if input symbols were consumed, @a false otherwise
    // @post @a read was incremented by the number of input symbols consumed
    bool skipNewline(size_t& read, script_info_t& script);
    struct linebuffer_t : buffer_t<MAXLINESIZE> {};

    linebuffer_t _linebuffer;

public:
    Ego::Script::Buffer _loadBuffer;

    /// @brief Analyse the contents of a string.
    /// @param string the string with the leading and trailing quotation marks stripped
    void parse_string(std::string string, Token& token, script_info_t& script, ObjectProfile *ppro);

    /**
    * @brief
    *  Get the error variable value.
    * @return
    *  the error variable value
    */
    bool get_error() const;
    /**
    * @brief
    *  Clear the error variable.
    */
    void clear_error();

private:
	static void surround_space(size_t position, linebuffer_t& buffer);
	static size_t fix_operators(linebuffer_t& buffer);
	void emit_opcode(Token& tok, const BIT_FIELD highbits, script_info_t& script);

	static Uint32 jump_goto(int index, int index_end, script_info_t& script);
public:
	static void parse_jumps(script_info_t& script);

private:
	size_t parse_token(Token& tok, ObjectProfile *ppro, script_info_t& script, size_t read);
	size_t load_one_line(size_t read, script_info_t& script);
	/**
	 * @brief
	 *  Compute the indention level of a line.
	 * @remark
	 *  A line must begin with a number of spaces \f$2k\f$ where \f$k=0,1,\ldots$, otherwise an error shall be raised.
	 *  The indention level $j$ of a line with \f$2k\f$ leading spaces for some $k=0,1,\ldots$ is given by \f$\frac{2k
	 *  }{2}=k\f$. The indention level \f$j\f$ of a line may not exceed \f$15\f$.
	 */
	int get_indentation(script_info_t& script);
public:
	void parse_line_by_line(ObjectProfile *ppro, script_info_t& script);

};

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	Load an AI script.
 * @param parser
 *	the parser
 * @param loadname
 *  the loadname
 * @param objectProfile
 *  the object profile
 * @param script
 * the script
 * @remark
 *  A call to this function tries to load the script.
 *		If this fails, then it tries to load the default script.
 *			If this fails, then the call to this function fails.
 */
egolib_rv load_ai_script_vfs(parser_state_t& ps, const std::string& loadname, ObjectProfile *ppro, script_info_t& script);
