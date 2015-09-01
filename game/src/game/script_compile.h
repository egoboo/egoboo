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
#include "game/egoboo_typedef.h"
#include "egolib/Script/script.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// AI stuff
#define AISMAXLOADSIZE      (1024*1024)         ///< For parsing AI scripts
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

extern StaticArray<opcode_data_t, MAX_OPCODE> OpList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A list of all possible egoscript functions
enum e_script_functions
{
#define Define(name) name,
#include "egolib/Script/Functions.in"
#undef Define
    SCRIPT_FUNCTIONS_COUNT
};
extern const char *script_function_names[SCRIPT_FUNCTIONS_COUNT];

/// A list of all possible egoscript variables
enum e_script_variables
{
#define Define(name) name,
#include "egolib/Script/Variables.in"
#undef Define
	SCRIPT_VARIABLES_COUNT
};
extern const char *script_variable_names[SCRIPT_VARIABLES_COUNT];

/// A list of all possible egoscript operators
enum e_script_operators
{
#define Define(name) name,
#include "egolib/Script/Operators.in"
#undef Define
	SCRIPT_OPERATORS_COUNT
};
extern const char *script_operator_names[SCRIPT_OPERATORS_COUNT];

//--------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------

// the current state of the parser
struct parser_state_t
{
private:
    /**
     * @brief
     *  A pointer to the singleton instance of the parser.
     */
    static parser_state_t *_singleton;
	/**
	 * @brief
	 *	Construct this parser.
	 * @remark
	 *	Intentionally protected.
	 */
	parser_state_t();
	/**
	 * @brief
	 *	Destruct this parser.
	 * @remark
	 *	Intentionally protected.
	 */
	virtual ~parser_state_t();
public:
    bool error;
    Token token;
    int line_count;

    size_t line_buffer_count;
    char line_buffer[MAXLINESIZE];

    size_t load_buffer_count;
    Uint8 load_buffer[AISMAXLOADSIZE];

    /**
     * @brief
     *  Initialize the singleton.
     * @return
     *  a pointer to the singleton if the singleton is initialized, @a nullptr on failure
     * @remark
     *  This function has no effect if the singeton is already initialized.
     */
    static parser_state_t *initialize();
    /**
     * @brief
     *  Uninitialize the singleton.
     * @remark
     *  This function has no effect is the singleton is already uninitialized.
     */
    static void uninitialize();
    /**
     * @brief
     *  Get a pointer to the singleton.
     * @throw std::logic_error
	 *	if the parser singleton is not initialized
     */
    static parser_state_t& get();


    /**
    * @brief
    *  Get the error variable value.
    * @return
    *  the error variable value
    */
    static bool get_error(parser_state_t& self);
    /**
    * @brief
    *  Clear the error variable.
    */
    static void clear_error(parser_state_t& self);

	static size_t parse_token(parser_state_t& self, Token& tok, ObjectProfile *ppro, script_info_t *pscript, size_t read);
	static size_t load_one_line(parser_state_t& self, size_t read, script_info_t *pscript);
	static int get_indentation(parser_state_t& self, script_info_t *pscript);
	static void parse_line_by_line(parser_state_t& self, ObjectProfile *ppro, script_info_t *pscript);

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// function prototypes


/**
 * @brief
 *	Load an AI script.
 * @param parser
 *	the parser
 */
egolib_rv load_ai_script_vfs(parser_state_t& ps, const std::string& loadname, ObjectProfile *ppro, script_info_t *pscript);
