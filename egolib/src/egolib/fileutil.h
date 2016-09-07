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

/// @file  egolib/fileutil.h
/// @brief read/write values from/to files

#pragma once

#include "egolib/typedef.h"
#include "egolib/Script/EnumDescriptor.hpp"
#include "egolib/vfs.h"
#include "egolib/Script/Token.hpp"
#include "egolib/Script/Traits.hpp"
#include "egolib/Log/_Include.hpp"
#include "egolib/file_common.h"
#include "egolib/Logic/Damage.hpp"
#include "egolib/IDSZ.hpp"
#include "egolib/Profiles/LocalParticleProfileRef.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "egolib/Renderer/Renderer.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egolib/Script/EnumDescriptor.hpp"
#include "egolib/Script/AbstractReader.hpp"
#include "egolib/Script/Errors.hpp"

/**
 * @brief
 *  A context for reading a file.
 * @details
 *  A context is associated with a load name (i.e. a name of a file in the virtual file system of Egoboo) from
 *  which it can read data. To read data, the context must be opened and if it is is sucessfully opened, it
 *  can be used to read data from that file. In particular, the associoated file does not have to exist until the
 *  point at which the context is opened.
 * @author
 *  Michael Heilmann
 */
struct ReadContext : public Ego::Script::AbstractReader<Ego::Script::Traits<char>>
{

public:

    /**
     * @brief
     *  The load name of the file of this context.
     */
    std::string _loadName;
    
    using Traits = Ego::Script::Traits<char>;

    ReadContext(const std::string& loadName);

    ~ReadContext();

    /**
     * @brief
     *  Convert the contents of the buffer to a float value.
     * @return
     *  the float value
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     */
    float toReal() const;

    /**
     * @brief
     *  Get the load name of the file associated with this context.
     * @return
     *  the load name of the file associated with this context
     */
    const std::string& getLoadName() const;

    /**
     * @brief
     *  Get if the context is open.
     * @return
     *  @a true if the context is open, @a false otherwise
     */
    bool isOpen() const;

    /**
     * @brief
     *  Open this context.
     * @return
     *  @a true if the context is open, @a false otherwise
     */
    bool ensureOpen();

    /**
     * @brief
     *  Close the context.
     * @post
     *  The context is closed.
     * @remark
     *  If the context is not open, a call to this method is noop.
     */
    void close();

    /**
     * @brief
     *  Read an enumeration.
     * @param enumDescriptor
     *  the enum descriptor to be used
     * @return
     *  the enumeration value
     * @todo
     *  Add comment about lexical error.
     */
    template <typename EnumType>
    EnumType readEnum(const Ego::Script::EnumDescriptor<EnumType>& enumDescriptor)
    {
        using namespace std;
        auto name = readName();
        auto it = enumDescriptor.find(name);
        if (it == enumDescriptor.end())
        {
            throw Id::LexicalErrorException(__FILE__,__LINE__,Ego::Script::Location(_loadName, _lineNumber), "invalid enum");
        }
        return it->second;
    }

    /**
     * @brief
     *  Read an enumeration.
     * @param enumDescriptor
     *  the enum descriptor to be used
     * @return
     *  the enumeration value
     * @todo
     *  Add comment about lexical error and default.
     */
    template <typename EnumType>
    EnumType readEnum(const Ego::Script::EnumDescriptor<EnumType>& enumDescriptor, EnumType defaultValue)
    {
        using namespace std;
        auto name = readName();
        auto it = enumDescriptor.find(name);
        if (it == enumDescriptor.end())
        {
			Log::get().warn("%s:%d: in file `%s`: `%s` is not an element of enum `%s`\n", __FILE__, __LINE__,
                            _loadName.c_str(), name.c_str(), enumDescriptor.getName().c_str());
            return defaultValue;
        }
        return it->second;
    }

    /// The new line character.
    static const int LineFeed = '\n';
    /// The carriage return character.
    static const int CarriageReturn = '\r';
    /// The zero terminator character.
    static const int ZeroTerminator = '\0';
    /// The tabulator character.
    static const int Tabulator = '\t';
    /// The space character.
    static const int Space = ' ';

    /**
     * @brief
     *  Read everything from the current character to the next newline character or end of file.
     *  Whitespaces in front and the newline character are dropped.
     */
    std::string readToEndOfLine();
    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A single line comment in this revision are the strings
     *  @code
     *  singleLineComment := '//'(any - NewLine)* 
     *  @endcode
     */
    std::string readSingleLineComment();

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A printable character in this revision are the strings
     *  @code
     *  verbatimChar := alphabetic | digit | '!' | '?' | '='
     *  @endcode
     */
    char readPrintable();

public:

	/**
	 * @remark
	 *  A string literal in this revision are the strings
	 *  @code
	 *  stringLiteral := (character|digit|'~'|'_')*
	 *  @endcode
	 */
	Ego::Script::TextToken parseStringLiteral();

	/**
	 * @throw Id::LexicalErrorException
	 *  if a lexical error occurs
	 * @remark
	 *  A character literal in this revision are the strings
	 *  @code
	 *  charLit := '\'' charLitBody '\''
	 *  charLitBody := (any - '\'') | ('\\n' | '\\t' | '\\\'')
	 *  @endcode
	 * @todo
	 *  Use Unicode notation for better readbility.
	 */
	Ego::Script::TextToken parseCharacterLiteral();

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  An integer literal in this revision are the strings
     *  @code
     *  integer := ('+'|'-')? digit+ ('e'|'E' digit+)?
     *  @endcode
     */
	Ego::Script::TextToken parseIntegerLiteral();

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A natural literal in this revision are the strings
     *  @code
     *  integer := '+'? digit+ ('e'|'E' digit+)?
     *  @endcode
     */
	Ego::Script::TextToken parseNaturalLiteral();

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A real literal in this revision are the strings
     *  @code
     *  float := ('+'|'-')? digit+ ('.' digit)* realExponent?
     *        | ('+'|'-')? '.' digit+ realExponent?
     *  realExponent := ('e'|'E' ('+'|'-')? digit+)?
     *  @endcode
     */
	Ego::Script::TextToken parseRealLiteral();

public:

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *   A boolean literal in this revision are the strings
     *   @code
     *   boolean := 'T' | 'F' | 'true' | 'false'
     *   @endcode
     */
    bool readBool();

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *   A name in this revision are the strings
     *   @code
     *   name := (alphabetic|underscore) (digit|alphabetic|underscore)*
     *   @endcode
     */
    void readName0();
    std::string readName();

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A reference in this revision are the strings
     *  @code
     *  reference := '%' name
     *  @endcode
     */
    void readReference0();
    std::string readReference();

    /**
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  An IDSZ literal in this revision is a string
     *  @code
     *  idsz := '[' (alphabetic|digit|'_')^4 ']'
     *  @endcode
     */
    IDSZ2 readIDSZ();

    /**
     * @brief
     *  Skip input until
     *  a non-whitespace
     *  extended character is the current character.
     */
    void skipWhiteSpaces();

    /**
     * @brief
     *  Skip input until either the end of the input or the specified delimiter is reached.
     *  If the specified delimiter is reached, skip it as well.
     * @param option
     *  if @a true and the specified delimiter was not encountered, a lexical error is raised
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @return
     *  @a true if the specifed delimiter is was skipped, @a false otherwise
     */
    bool skipToDelimiter(char delimiter, bool optional);

    /**
     * @brief
     *  Skip input until either the end of the input or a colon is reached.
     *  If a colon is reached, skip it as well.
     * @param option
     *  if @a true and a colon was not encountered, a lexical error is raised
     * @throw Id::LexicalErrorException
     *  if a lexical error occurs
     * @return
     *  @a true if a colon was skipped, @a false otherwise
     */
    bool skipToColon(bool optional);

    //Disable copying class
    ReadContext(const ReadContext& copy) = delete;
    ReadContext& operator=(const ReadContext&) = delete;

public:

	std::string readStringLiteral();
	char readCharacterLiteral();
	int readIntegerLiteral();
	unsigned int readNaturalLiteral();
	float readRealLiteral();
};

// Utility functions.
void vfs_get_next_name(ReadContext& ctxt, char *buf, size_t max);
bool vfs_get_next_range(ReadContext& ctxt, FRange *range);
bool vfs_get_next_pair(ReadContext& ctxt, IPair *pair);
IDSZ2 vfs_get_next_idsz(ReadContext& ctxt);
bool vfs_get_next_bool(ReadContext& ctxt);
int32_t vfs_get_next_int32(ReadContext& ctxt);
void vfs_get_next_string_lit(ReadContext& ctxt, char *str, size_t max);
UFP8_T vfs_get_ufp8(ReadContext& ctxt);
SFP8_T vfs_get_sfp8(ReadContext& ctxt);
char vfs_get_next_printable(ReadContext& ctxt);
signed int vfs_get_next_int(ReadContext& ctxt);
unsigned int vfs_get_next_nat(ReadContext& ctxt);
float vfs_get_next_float(ReadContext& ctxt);
UFP8_T vfs_get_next_ufp8(ReadContext& ctxt);
SFP8_T vfs_get_next_sfp8(ReadContext& ctxt);
LocalParticleProfileRef vfs_get_next_local_particle_profile_ref(ReadContext& ctxt);

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

int    vfs_get_version(ReadContext& ctxt);
LocalParticleProfileRef vfs_get_local_particle_profile_ref(ReadContext& ctxt);
DamageType vfs_get_damage_type(ReadContext& ctxt);
DamageModifier vfs_get_damage_modifier(ReadContext& ctxt);
float  vfs_get_damage_resist(ReadContext& ctxt);
/**
 * @brief
 *  Read a name (transitional C form).
 * @param buf, max
 *  @a buf is a pointer to a buffer of <tt>max+1</tt> bytes
 * @see
 *  ReadContext::readName
 * @todo
 *  Transitional C form, remove this.
 */
void vfs_read_name(ReadContext& ctxt, char *buf, size_t max);
/**
 * @brief
 *  Read a string (transitional C form).
 * @param buf, max
 *  @a buf is a pointer to a buffer of <tt>max+1</tt> bytes
 * @see
 *  ReadContext::readString
 * @todo
 *  Transitional C form, remove this.
 */
void vfs_read_string_lit(ReadContext& ctxt, char *buf, size_t max);
/**
 * @brief Read a string.
 * @param [out] receiver
 * @see ReadContext::readString
 * @remark A consecutive sequence of characters <tt>'_'</tt> is mapped to <tt>' '</tt> and <tt>'~'</tt> is mapped to <tt>'\\t'</tt>.
 */
void vfs_read_string_lit(ReadContext& ctxt, std::string& literal);
/**
 * @brief
 *  Read a range.
 * @remark
 *  In this revision a range are the strings
 *  @code
 *  range := real ('-' real)?
 *  @endcode
 */
bool vfs_get_range(ReadContext& ctxt, FRange *range);
/**
 * @brief
 *  Read a pair.
 * @remark
 *  In this revision a pair are the strings
 *  @code
 *  pair := range
 *  @endcode
 */
bool vfs_get_pair(ReadContext& ctxt, IPair *pair);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

DamageType vfs_get_next_damage_type(ReadContext& ctxt);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Stuff to remove.

bool read_to_delimiter_vfs(ReadContext& ctxt, std::string& buffer, char delimiter, bool optional);
bool read_to_colon_vfs(ReadContext& ctxt, std::string& buffer, bool optional);
bool read_to_delimiter_list_vfs(ReadContext& ctxt, std::string& buffer, const char *delimiters, bool optional);

void make_newloadname(const char *modname, const char *appendname, char *newloadname);

bool copy_line_vfs(vfs_FILE * fileread, vfs_FILE * filewrite);
char vfs_get_first_letter(ReadContext& ctxt);
char * copy_to_delimiter_mem(char * pmem, char * pmem_end, vfs_FILE * filewrite, int delim, char * user_buffer, size_t user_buffer_len);
bool copy_to_delimiter_vfs(vfs_FILE * fileread, vfs_FILE * filewrite, int delim, char * buffer, size_t bufflen);

bool ego_texture_exists_vfs(const std::string &filename);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Stuff to encapsulte in a WriterContext.
bool vfs_put_version(vfs_FILE* filewrite, const int version);
void vfs_put_int(vfs_FILE* filewrite, const char* text, int value);
void vfs_put_int32(vfs_FILE *filewrite, const char* text, int32_t value);
void vfs_put_float(vfs_FILE* filewrite, const char* text, float value);
void vfs_put_ufp8(vfs_FILE* filewrite, const char* text, UFP8_T value);
void vfs_put_sfp8(vfs_FILE* filewrite, const char* text, SFP8_T value);
void vfs_put_bool(vfs_FILE* filewrite, const char* text, bool value);
void vfs_put_damage_type(vfs_FILE* filewrite, const char* text, Uint8 value);
void vfs_put_action(vfs_FILE* filewrite, const char* text, Uint8 value);
void vfs_put_gender_profile(vfs_FILE* filewrite, const char* text, GenderProfile value);
void vfs_put_range(vfs_FILE* filewrite, const char* text, FRange value);
void vfs_put_pair(vfs_FILE* filewrite, const char* text, IPair value);
void vfs_put_string_under(vfs_FILE* filewrite, const char* text, const char* usename);
void vfs_put_idsz(vfs_FILE* filewrite, const char* text, const IDSZ2& idsz);
void vfs_put_expansion(vfs_FILE *filewrite, const char *text, const IDSZ2& idsz, int value);
void vfs_put_expansion(vfs_FILE *filewrite, const char *text, const IDSZ2& idsz, const LocalParticleProfileRef& lppref);
void vfs_put_expansion_float(vfs_FILE* filewrite, const char* text, const IDSZ2& idsz, float value);
void vfs_put_expansion_string(vfs_FILE* filewrite, const char* text, const IDSZ2& idsz, const char * value);
void vfs_put_range_raw(vfs_FILE* filewrite, FRange val);
void vfs_put_local_particle_profile_ref(vfs_FILE *filewrite, const char *text, const LocalParticleProfileRef& lppref);
