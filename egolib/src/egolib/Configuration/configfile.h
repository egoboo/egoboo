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

/// @file egolib/Configuration/configfile.h
/// @brief Parsing and unparsing of configuration files

#pragma once

#include "egolib/vfs.h"
#include "egolib/Script/Traits.hpp"
#include "egolib/Script/QualifiedName.hpp"
#include "egolib/Script/AbstractReader.hpp"
#include "egolib/Script/Buffer.hpp"
#include "egolib/Script/TextInputFile.hpp"

using namespace std;
using namespace Ego::Script;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct AbstractConfigFile
{
public:
    /// @brief A single comment line.
    struct CommentLine
    {
        string text;
        CommentLine(const string& text) :
            text(text)
        {}
        virtual ~CommentLine()
        {}
        const string& getText() const
        {
            return text;
        }
    };

    /// @brief A single entry in a configuration file.
    struct Entry
    {
    protected:
        /// @brief The comment lines associated with this entry.
        vector<CommentLine> commentLines;

        /// @brief The qualified name of this entry.
        QualifiedName qualifiedName;

        /// @brief The value of this entry.
        string value;

    public:

        /// @brief Construct this entry.
        /// @param qualifiedName the qualified name
        /// @param value the value
        Entry(const QualifiedName& qualifiedName, const string& value) :
            qualifiedName(qualifiedName), value(value),
            commentLines()
        {}

        /// @brief Destruct this entry.
        virtual ~Entry()
        {}

        /// @brief Get the qualified name of this entry.
        /// @return the qualified name of this entry
        const QualifiedName& getQualifiedName() const
        {
            return qualifiedName;
        }

        /// @brief Get the value of this entry.
        /// @return the value of this entry
        const string& getValue() const
        {
            return value;
        }

        /// @brief Get the comment lines of this entry.
        /// @return the comment lines of this entry
        const vector<CommentLine>& getCommentLines() const
        {
            return commentLines;
        }
    };

public:

    using MapTy = unordered_map<QualifiedName, shared_ptr<Entry>>;
    using ConstMapIteratorTy = MapTy::const_iterator;

    /// @internal Custom iterator.
    struct EntryIterator : iterator<forward_iterator_tag, const shared_ptr<Entry>>
    {
    public:
        using iterator_category = forward_iterator_tag;

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
    string _fileName;

    /**
     * @brief
     *  A map of qualified names (keys) to shared pointers of entries (values).
     */
    unordered_map<QualifiedName,shared_ptr<Entry>> _map;

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
    AbstractConfigFile(const string& fileName) :
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
    const string& getFileName() const
    {
        return _fileName;
    }

    /**
     * @brief
     *  Set the file name of this configuration file.
     * @param fileName
     *  the file name
     */
    void setFileName(const string& fileName)
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
    bool set(const QualifiedName& qn, const string& v)
    {
        try
        {
            _map[qn] = make_shared<Entry>(qn,v);
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
    bool get(const QualifiedName& qn, string& v) const
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
    ConfigFile(const string& fileName) :
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
//--------------------------------------------------------------------------------------------

struct ConfigFileUnParser
{
protected:
    /**
     * @brief
     *  The configuration file.
     */
    shared_ptr<ConfigFile> _source;
        
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
    bool unparse(shared_ptr<ConfigFile::Entry> entry);

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
    bool unparse(shared_ptr<ConfigFile> source);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ConfigFileParser : public AbstractReader<Traits<char>>
{
public:

    using Traits = Ego::Script::Traits<char>;

protected:
    /**
     * @brief
     *  The current qualified name or @a nullptr.
     */
    unique_ptr<QualifiedName> _currentQualifiedName;
    /**
     * @brief
     *  The current value or @a nullptr.
     */
    unique_ptr<string> _currentValue;

    vector<string> _commentLines;

    /**
     * @brief
     *  Flush the comment buffer:
     *  Concatenate all comments in the buffer to a single string and empty the buffer.
     */
    string makeComment();

public:

    ConfigFileParser(const std::string& fileName);

    virtual ~ConfigFileParser();


    shared_ptr<ConfigFile> parse();

protected:

    /**
     * @remark
     *  @code
     *  file -> entries*
     *  @endcode
     */
    bool parseFile(shared_ptr<ConfigFile> target);

    /**
     * @remark
     *  @code
     *  entry -> sectionName keyValuePair*
     *  @endcode
     */
    bool parseEntry(shared_ptr<ConfigFile> target);

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

    bool parseComment(string& comment);

    bool skipWhiteSpaces();

};
