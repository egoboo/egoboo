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

/// @file egolib/FileFormats/ConfigFile/configfile.h
/// @brief Parsing and unparsing of configuration files

#pragma once

#include "egolib/vfs.h"
#include "egolib/Script/Traits.hpp"
#include "egolib/Script/Scanner.hpp"

//--------------------------------------------------------------------------------------------

/// @brief A single comment line.
struct ConfigCommentLine
{
private:
    std::string m_text;

public:
    ConfigCommentLine(const std::string& text);

    virtual ~ConfigCommentLine();

    const std::string& getText() const;
};

//--------------------------------------------------------------------------------------------

/// @brief A single entry in a configuration file.
struct ConfigEntry
{
protected:
    /// @brief The comment lines associated with this entry.
    std::vector<ConfigCommentLine> m_commentLines;

    /// @brief The qualified name of this entry.
    idlib::c::qualified_name m_qualifiedName;

    /// @brief The value of this entry.
    std::string m_value;

public:
    /// @brief Construct this entry.
    /// @param qualifiedName the qualified name
    /// @param value the value
    ConfigEntry(const idlib::c::qualified_name& qualifiedName, const std::string& value);

    /// @brief Destruct this entry.
    virtual ~ConfigEntry();

    /// @brief Get the qualified name of this entry.
    /// @return the qualified name of this entry
    const idlib::c::qualified_name& getQualifiedName() const;

    /// @brief Get the value of this entry.
    /// @return the value of this entry
    const std::string& getValue() const;

    /// @brief Get the comment lines of this entry.
    /// @return the comment lines of this entry
    const std::vector<ConfigCommentLine>& getCommentLines() const;
};

//--------------------------------------------------------------------------------------------

struct AbstractConfigFile
{
public:

    using MapTy = std::unordered_map<idlib::c::qualified_name, std::shared_ptr<ConfigEntry>>;
    using ConstMapIteratorTy = MapTy::const_iterator;

    /// @internal Custom iterator.
    struct EntryIterator : std::iterator<std::forward_iterator_tag, const std::shared_ptr<ConfigEntry>>
    {
    public:
        using iterator_category = std::forward_iterator_tag;

        using OuterIteratorTy = ConstMapIteratorTy;

    private:
        OuterIteratorTy _inner;
    
    public:
        
        EntryIterator() :
            _inner()
        {}
        
        EntryIterator(const OuterIteratorTy& outer) :
            _inner(outer)
        {}
        
        EntryIterator(const EntryIterator& other) :
            _inner(other._inner)
        {}
        
        reference operator->() const
        {
            return _inner->second;
        }
        
        reference operator*() const
        {
            return _inner->second;
        }

        bool operator!=(const EntryIterator& other) const
        {
            return _inner != other._inner;
        }

        bool operator==(const EntryIterator& other) const
        {
            return _inner == other._inner;
        }

#if 0
        // Prefix decrement.
        EntryIterator& operator--()
        {
            --_inner;
            return *this;
        }
#endif

        // Prefix increment.
        EntryIterator& operator++()
        {
            ++_inner;
            return *this;
        }

#if 0
        // Postfix decrement.
        EntryIterator operator--(int)
        {
            EntryIterator t = *this;
            --_inner;
            return t;
        }
#endif

        // Postfix increment.
        EntryIterator operator++(int)
        {
            EntryIterator t = *this;
            ++_inner;
            return t;
        }

        EntryIterator& operator=(const EntryIterator& other)
        {
            _inner = other._inner;
            return *this;
        }

    };

    /**
     * @brief
     *  The file name of the configuration file.
     */
    std::string _fileName;

    /**
     * @brief
     *  A map of qualified names (keys) to shared pointers of entries (values).
     */
    std::unordered_map<idlib::c::qualified_name,std::shared_ptr<ConfigEntry>> _map;

public:

    // STL-style
    using iterator = EntryIterator;
    // STL-style
    using const_iterator = EntryIterator;

    /**
     * @brief
     *  Construct this configuration.
     * @param fileName
     *  the file name
     * @post
     *  The configuration file is empty and has the specified file name.
     */
    AbstractConfigFile(const std::string& fileName) :
        _fileName(fileName), _map()
    {}

    /**
     * @brief
     *  Destruct this configuration file.
     */
    virtual ~AbstractConfigFile()
    {}

    /**
     * @brief
     *  Get an iterator to the beginning of the entries.
     * @return
     *  the iterator
     */
    iterator begin() const
    {
        return EntryIterator(_map.begin());
    }

    /**
     * @brief
     *  Get an iterator to the end of the entries.
     * @return
     *  the iterator
     */
    iterator end() const
    {
        return EntryIterator(_map.end());
    }

    /**
     * @brief
     *  Get the file name of this configuration file.
     * @return
     *  the file name
     */
    const std::string& getFileName() const
    {
        return _fileName;
    }

    /**
     * @brief
     *  Set the file name of this configuration file.
     * @param fileName
     *  the file name
     */
    void setFileName(const std::string& fileName)
    {
        _fileName = fileName;
    }

public:

    /**
     * @brief
     *  Set an entry.
     * @param qn
     *  the qualified name
     * @param v
     *  the value
     * @return
     *  @a true on success, @a false on failure
     */
    bool set(const idlib::c::qualified_name& qn, const std::string& v)
    {
        try
        {
            _map[qn] = std::make_shared<ConfigEntry>(qn,v);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
    
    /**
     * @brief
     *  Get a value.
     * @param qn
     *  the qualified name
     * @param [out] v
     *  reference to a variable in which the value is stored
     * @return
     *  @a true if the value exists, @a false otherwise.
     *  If @a true is returned, the value was stored @a v.
     *  the value if it exists, @a nullptr otherwise
     */
    bool get(const idlib::c::qualified_name& qn, std::string& v) const
    {
        auto it = _map.find(qn);
        if (it != _map.end())
        {
            v = (*it).second->getValue();
            return true;
        }
        else
        {
            return false;
        }
    }

};


/**
 * @brief
 *  A configuration file.
 * @todo
 *  Refactor to @a ConfigFile.
 */
struct ConfigFile : public AbstractConfigFile
{

public:

    /**
     * @brief
     *  Construct this configuration.
     * @param fileName
     *  the file name
     * @post
     *  The configuration file is empty and has the specified file name.
     */
    ConfigFile(const std::string& fileName) :
        AbstractConfigFile(fileName)
    {}

    /**
     * @brief
     *  Destruct this configuration file.
     */
    virtual ~ConfigFile()
    {}

};

//--------------------------------------------------------------------------------------------

struct ConfigFileUnParser
{
protected:
    /**
     * @brief
     *  The configuration file.
     */
    std::shared_ptr<ConfigFile> _source;
        
    /**
     * @brief
     *  The target file.
     */
    vfs_FILE *_target;

    /**
     * @brief
     *  Unparse an entry.
     * @param entry
     *  the entry
     * @return
     *  @a true on success, @a false on failure
     */
    bool unparse(std::shared_ptr<ConfigEntry> entry);

public:
    /**
     * @brief
     *  Construct this configuration file unparser.
     */
    ConfigFileUnParser();
    /**
     * @brief
     *  Destruct this configuration file unparser.
     */
    virtual ~ConfigFileUnParser();
    /**
     * @brief
     *  Save the configuration file.
     * @param source
     *  the configuration file
     */
    bool unparse(std::shared_ptr<ConfigFile> source);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ConfigFileParser : public Ego::Script::Scanner<Ego::Script::Traits<char>>
{
public:

    using Traits = Ego::Script::Traits<char>;

protected:
    /**
     * @brief
     *  The current qualified name or @a nullptr.
     */
    std::unique_ptr<idlib::c::qualified_name> _currentQualifiedName;
    /**
     * @brief
     *  The current value or @a nullptr.
     */
    std::unique_ptr<std::string> _currentValue;

    std::vector<std::string> _commentLines;

    /**
     * @brief
     *  Flush the comment buffer:
     *  Concatenate all comments in the buffer to a single string and empty the buffer.
     */
    std::string makeComment();

public:

    ConfigFileParser(const std::string& fileName);

    virtual ~ConfigFileParser();


    std::shared_ptr<ConfigFile> parse();

protected:

    /**
     * @remark
     *  @code
     *  file -> entries*
     *  @endcode
     */
    bool parseFile(std::shared_ptr<ConfigFile> target);

    /**
     * @remark
     *  @code
     *  entry -> sectionName keyValuePair*
     *  @endcode
     */
    bool parseEntry(std::shared_ptr<ConfigFile> target);

    /**
     * @remark
     *  @code
     *  variable -> qualifiedName ':' value
     *              {
     *                  @variables.append(Variable::new(@qualifiedName,@string))
     *              }
     *  @endcode
     */
    bool parseVariable();

    /**
     * @remark
     *  @code
     *  qualifiedName -> (name('.' name)*)@qualifiedName
     *                   {
     *                     @currentQualifiedName := @qualifiedName
     *                   }
     *  @endcode
     */
    bool parseQualifiedName();

    /**
     * @remark
     *  @code
     *  name -> '_' * alphabetic (alphabetic | digit | '_')
     *  @endcode
     */
    bool parseName();

    /**
     * @remark
     *  @code
     *  value := '"' (valueChar*)@value '"'
     *         {
     *           @currentValue := @value
     *         }
     *  valueChar := . - (endOfInput | error | '"' | newLine)
     *  @endcode
     *  where @code{'.'} is any character. 
     */
    bool parseValue();

    bool parseComment(std::string& comment);

    bool skipWhiteSpaces();

};
