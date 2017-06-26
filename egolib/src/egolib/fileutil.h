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
#include "egolib/Script/DDLToken.hpp"
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
#include "egolib/Script/Scanner.hpp"
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
struct ReadContext : public Ego::Script::Scanner<Ego::Script::Traits<char>>
{

public:   
    using Traits = Ego::Script::Traits<char>;

    ReadContext(const std::string& fileName);

    ~ReadContext();

    /**
     * @brief
     *  Convert the contents of the buffer to a float value.
     * @return
     *  the float value
     * @throw LexicalErrorException
     *  if a lexical error occurs
     */
    float toReal() const;

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
            throw id::compilation_error(__FILE__, __LINE__, id::compilation_error_kind::lexical, get_location(), "invalid enum");
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
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "in file ", "`", get_file_name(), "`", ":", "`", name, "`",
                                             " is not an element of enum ", "`", enumDescriptor.getName(), "`", Log::EndOfEntry);
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
     * @throw LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A single line comment in this revision are the strings
     *  @code
     *  singleLineComment := '//'(any - NewLine)* 
     *  @endcode
     */
    std::string readSingleLineComment();

    /**
     * @throw LexicalErrorException
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
    Ego::Script::DDLToken parseStringLiteral();

	/**
	 * @throw LexicalErrorException
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
    Ego::Script::DDLToken parseCharacterLiteral();

    /**
     * @throw LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  An integer literal in this revision are the strings
     *  @code
     *  integer := ('+'|'-')? digit+ ('e'|'E' digit+)?
     *  @endcode
     */
    Ego::Script::DDLToken parseIntegerLiteral();

    /**
     * @throw LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A natural literal in this revision are the strings
     *  @code
     *  integer := '+'? digit+ ('e'|'E' digit+)?
     *  @endcode
     */
    Ego::Script::DDLToken parseNaturalLiteral();

    /**
     * @throw LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *  A real literal in this revision are the strings
     *  @code
     *  float := ('+'|'-')? digit+ ('.' digit)* realExponent?
     *        | ('+'|'-')? '.' digit+ realExponent?
     *  realExponent := ('e'|'E' ('+'|'-')? digit+)?
     *  @endcode
     */
    Ego::Script::DDLToken parseRealLiteral();

public:

    /**
     * @throw LexicalErrorException
     *  if a lexical error occurs
     * @remark
     *   A boolean literal in this revision are the strings
     *   @code
     *   boolean := 'T' | 'F' | 'true' | 'false'
     *   @endcode
     */
    bool readBool();

    /**
     * @throw LexicalErrorException
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
     * @throw LexicalErrorException
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
     * @throw LexicalErrorException
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
     * @throw LexicalErrorException
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
     * @throw LexicalErrorException
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
std::string vfs_get_next_name(ReadContext& ctxt);
Ego::Math::Interval<float> vfs_get_next_range(ReadContext& ctxt);
IDSZ2 vfs_get_next_idsz(ReadContext& ctxt);
bool vfs_get_next_bool(ReadContext& ctxt);
int32_t vfs_get_next_int32(ReadContext& ctxt);
std::string vfs_get_next_string_lit(ReadContext& ctxt);
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
std::string vfs_read_name(ReadContext& ctxt);
/**
 * @brief Read a string.
 * @param ctxt the context
 * @param [out] literal a reference to a std::string receiving the literal
 * @see ReadContext::readString
 * @remark A consecutive sequence of characters <tt>'_'</tt> is mapped to <tt>' '</tt> and <tt>'~'</tt> is mapped to <tt>'\\t'</tt>.
 */
std::string vfs_read_string_lit(ReadContext& ctxt);
/**
 * @brief
 *  Read a range.
 * @remark
 *  In this revision a range are the strings
 *  @code
 *  range := real ('-' real)?
 *  @endcode
 */
Ego::Math::Interval<float> vfs_get_range(ReadContext& ctxt);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

DamageType vfs_get_next_damage_type(ReadContext& ctxt);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Stuff to remove.

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
void vfs_put_damage_type(vfs_FILE* filewrite, const char* text, uint8_t value);
void vfs_put_action(vfs_FILE* filewrite, const char* text, uint8_t value);
void vfs_put_gender_profile(vfs_FILE* filewrite, const char* text, GenderProfile value);
void vfs_put_range(vfs_FILE* filewrite, const char* text, Ego::Math::Interval<float> value);
void vfs_put_pair(vfs_FILE* filewrite, const char* text, IPair value);
void vfs_put_string_under(vfs_FILE* filewrite, const char* text, const char* usename);
void vfs_put_idsz(vfs_FILE* filewrite, const char* text, const IDSZ2& idsz);
void vfs_put_expansion(vfs_FILE *filewrite, const char *text, const IDSZ2& idsz, int value);
void vfs_put_expansion(vfs_FILE *filewrite, const char *text, const IDSZ2& idsz, const LocalParticleProfileRef& lppref);
void vfs_put_expansion_float(vfs_FILE* filewrite, const char* text, const IDSZ2& idsz, float value);
void vfs_put_expansion_string(vfs_FILE* filewrite, const char* text, const IDSZ2& idsz, const char * value);
void vfs_put_range_raw(vfs_FILE* filewrite, Ego::Math::Interval<float> val);
void vfs_put_local_particle_profile_ref(vfs_FILE *filewrite, const char *text, const LocalParticleProfileRef& lppref);
