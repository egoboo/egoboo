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

/// @file egolib/Configuration/configfile.c
/// @brief Parsing and unparsing of configuration files

/**
 * @remark
 *  A configuration file contains sections which themselves contain key-value pairs.
 *  </br>
 *  A section name is contained between <tt>{}</tt>.
 *  Example: <tt>{Section Name}</tt>
 *  </br>
 *  A key-value pair has a key, a string value and an optional commentary.
 *  The key is contained between <tt>[]</tt>.
 *  Example: <tt>[Key Name]</tt>
 *  </br>
 *  The value is contained between <tt>"</tt>.
 *  Example: <tt>"true"</tt> or <tt>"Hello, World!"</tt> or <tt>"0.1"</tt>.
 *  To include a <tt>"</tt> in a string value, prefix the <tt>"</tt> by a <tt>\</tt>.
 *  To include a <tt>\</tt> in a string value, double the <tt>\</tt>.
 *  Example: <tt>"\\"</tt>.
 *  </br>
 *  A commentary begins with "//" and extends to the end of the line and is assigned to the key-value pair succeeding the it.
 */
/**
 * Example configuration file.:
 * @code
 * {Section 1}
 * // This is the comment for [Key1] : "TRUE".
 * [Key1] : "TRUE" // This is a commentary
 * // This is the comment for [Key2] : "Hello \"MAN\""
 * [Key2] : "Hello \"MAN\""
 * // This line and the next line are the comment for [Key2] : "\\xxx"
 * // which will become "\xxx".
 * [Key3] : "\\xxx"
 * @endcode
 */
/**
 * @bug
 *  Multiple section with the same name will be loaded and saved but only the first
 *  one will be looked for value. Should not load sections with same name.
 */
#include "egolib/Configuration/configfile.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool ConfigFileParser::skipWhiteSpaces()
{
    while (isWhiteSpace())
    {
        next();
    }
    return true;
}

bool ConfigFileParser::parseFile(shared_ptr<ConfigFile> target)
{
    assert(is(Traits::startOfInput()));
    _currentQualifiedName = nullptr;
    _currentValue = nullptr;
    next();
    while (Traits::endOfInput() != current())
    {
        if (isNewLine())
        {
            skipNewLines();
        }
        else if (isWhiteSpace())
        {
            skipWhiteSpaces();
        }
        else if (is('/'))
        {
            string comment;
            parseComment(comment);
            _commentLines.push_back(comment);
        }
        else if (!parseEntry(target))
        {
            return false;
        }
    }
    return true;
}

bool ConfigFileParser::parseEntry(shared_ptr<ConfigFile> target)
{
    if (!parseQualifiedName())
    {
        return false;
    }
    if (!skipWhiteSpaces())
    {
        return false;
    }
    if (!is(':'))
    {
        fprintf(stderr, "%s: invalid key-value pair\n", getFileName().c_str());
        return false;
    }
    next();
    if (!skipWhiteSpaces())
    {
        return false;
    }
    if (!parseValue())
    {
        return false;
    }
    target->set(*_currentQualifiedName, *_currentValue);
    // Throw current qualified name and current value away.
    _currentQualifiedName.reset(nullptr);
    _currentValue.reset(nullptr);
    return true;
}

bool ConfigFileParser::parseComment(std::string& comment)
{
    assert(is('/'));
    next();
    if (!is('/'))
    {
        fprintf(stderr, "%s: invalid comment\n", getFileName().c_str());
        return false;
    }
    next();
    _buffer.clear();
    while (!is(Traits::endOfInput()) && !is(Traits::error()) && !isNewLine())
    {
        saveAndNext();
    }
    skipNewLines();
    if (Traits::error() == current())
    {
        fprintf(stderr, "%s: error while reading file\n", getFileName().c_str());
        return false;
    }
    comment = toString();
    return true;
}

bool ConfigFileParser::parseName()
{
    assert(is('_') || isAlpha());
    while (is('_'))
    {
        saveAndNext();
    }
    if (!isAlpha())
    {
        fprintf(stderr, "%s: invalid name\n", getFileName().c_str());
        return false;
    }
    do
    {
        saveAndNext();
    } while (isAlpha() || isDigit() || is('_'));
    return true;
}

bool ConfigFileParser::parseQualifiedName()
{
    assert(is('_') || isAlpha());
    _buffer.clear();
    if (!parseName())
    {
        return false;
    }
    while (is('.'))
    {
        saveAndNext();
        if (!parseName())
        {
            return false;
        }
    }
    _currentQualifiedName.reset(new QualifiedName(toString()));

    return true;
}

bool ConfigFileParser::parseValue()
{
    assert(is('"'));
    next();
    _buffer.clear();
    while (Traits::isValid(current()) && !is('"') && !isNewLine())
    {
        saveAndNext();
    }
    if (!is('"'))
    {
        fprintf(stderr, "%s: invalid value\n", getFileName().c_str());
        return false;
    }
    next();
    _currentValue.reset(new string(toString()));
    return true;
}

ConfigFileParser::ConfigFileParser(const std::string& fileName) :
    AbstractReader(fileName, 512), _currentQualifiedName(nullptr), _currentValue(nullptr)
{}

ConfigFileParser::~ConfigFileParser()
{}

std::shared_ptr<ConfigFile> ConfigFileParser::parse()
{
    try
    {
        auto target = make_shared<ConfigFile>(getFileName());
        if (!parseFile(target))
        {
            return nullptr;
        }
        return target;
    }
    catch (...)
    {
        return nullptr;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool ConfigFileUnParser::unparse(shared_ptr<ConfigFile::Entry> entry)
{
    vfs_printf(_target,"%s : \"%s\"\n", entry->getQualifiedName().getString().c_str(),
                                        entry->getValue().c_str());
    return true;
}

ConfigFileUnParser::ConfigFileUnParser() :
    _target(nullptr), _source(nullptr)
{}

ConfigFileUnParser::~ConfigFileUnParser()
{}

bool ConfigFileUnParser::unparse(shared_ptr<ConfigFile> source)
{
    try
    {
        _source = source;
        _target = vfs_openWrite(_source->getFileName().c_str());
        if (!_target)
        {
            fprintf(stderr, "%s: unable to open file for writing\n", _source->getFileName().c_str());
            _source = nullptr;
            return false;
        }
        // Sort the entries by their qualified names.
        vector<shared_ptr<ConfigFile::Entry>> entries;
        for (const auto& entry : *_source)
        {
            entries.push_back(entry);
        }
        sort(entries.begin(), entries.end(),
             [] (const shared_ptr<ConfigFile::Entry>& a, const shared_ptr<ConfigFile::Entry>& b)
                {
                    return less<QualifiedName>()(a->getQualifiedName(), b->getQualifiedName());
                }
            );
        for (const auto& entry : entries)
        {
            unparse(entry);
        }
        _source = nullptr;
        vfs_close(_target);
        _target = nullptr;
    }
    catch (...)
    {
        _source = nullptr;
        if (_target)
        {
            vfs_close(_target);
            _target = nullptr;
        }
        return false;
    }
    return true;
}