// Copyright Michael Heilmann 2016, 2017.
//
// This file is part of Idlib.
//
// Idlib is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Idlib is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Idlib. If not, see <http://www.gnu.org/licenses/>.

/// @file idlib/script/tree_v1.hpp
/// @brief Abstract syntax for a EgoScript version 1 definitions.
#pragma once

#include "idlib/utility.hpp"
#include "idlib/language.hpp"

/// @brief The kinds of statements.
enum class statement_kind_v1
{
    /// @brief Unknown statement.
    unknown,
    /// @brief A block.
    block,
    /// @brief A line.
    line,
    /// @brief An empty line.
    empty_line,
    /// @brief A list of statements.
    statement_list,
    /// @brief A word.
    word,
}; // enum class statement_kind_v1

enum class node_kind_v1
{
    unknown,
    program,
    word,
    statement,
}; // enum class node_kind_v1

enum class word_kind_v1
{
    unknown,
    idsz,
    op,
    identifier,
    string,
    number
}; // enum class word_kind_v1

class node_v1 : public id::category_element<node_kind_v1, node_kind_v1::unknown>
{
protected:
    /// @brief A pointer to the parent node or a null pointer.
    std::weak_ptr<node_v1> m_parent;

    /// @brief The location.
    id::location m_location;
    
    /// @brief Construct this node.
    /// @param node_kind the node kind of this node
    /// @param location the location of this node
    /// @remark Intentionally protected.
    node_v1(node_kind_v1 node_kind, const id::location& location) :
        id::category_element<node_kind_v1, node_kind_v1::unknown>(node_kind),
        m_parent(), m_location(location)
    {}

public:
    /// @brief Destruct this node.
    virtual ~node_v1()
    {}
    
    /// @brief Get the location of this node.
    /// @return the location of this node
    const id::location& get_location() const
    {
        return m_location;
    }

    /// @brief Get the parent node of this node.
    /// @return a shared pointer to the parent node of this node if this node has a parent node, a null pointer otherwise
    std::shared_ptr<node_v1> get_parent() const
    { 
        return m_parent.lock();
    }
    
    /// @brief Set the parent node of this node.
    /// @param parent a shared pointer to the parent node or a null pointer
    void set_parent(const std::shared_ptr<node_v1>& parent)
    { 
        m_parent = parent;
    }

}; // class node_v1

/// @brief A word.
class word_v1 : public node_v1, public id::category_element<word_kind_v1, word_kind_v1::unknown>
{
private:
    std::string m_text;

public:
    /// @param word_kind the word kind of this word
    /// @param location the location of this word
    word_v1(word_kind_v1 word_kind, const id::location& location) :
        node_v1(node_kind_v1::word, location),
        id::category_element<word_kind_v1, word_kind_v1::unknown>(word_kind),
        m_text()
    {}

    void set_text(const std::string& text)
    { m_text = text; }

    const std::string& get_text() const
    { return m_text; }

}; // class word_v1

/// @brief A statement.
class statement_v1 : public node_v1, public id::category_element<statement_kind_v1, statement_kind_v1::unknown>
{
public:
	/// @brief Construct this statement.
	/// @param kind the statement kind of this statement
    /// @param location the location of this statement
	statement_v1(statement_kind_v1 statement_kind, const id::location& location) :
        node_v1(node_kind_v1::statement, location),
        id::category_element<statement_kind_v1, statement_kind_v1::unknown>(statement_kind)
	{ }

    /// @brief Destruct this statement.
    virtual ~statement_v1()
    {}
	
}; // class statement

/// @brief A statement list.
class statement_list_v1 : public statement_v1
{
private:
    /// @brief The statements.
    std::vector<std::shared_ptr<statement_v1>> m_statements;

public:
    /// @brief Construct this statement list.
    /// @param location the location of this statement list
    statement_list_v1(const id::location& location) :
        statement_v1(statement_kind_v1::statement_list, location),
        m_statements()
    {}

    /// @brief Set the statements.
    /// @param stats the statements
    void set_statements(const std::vector<std::shared_ptr<statement_v1>>& statements)
    {
        m_statements = statements;
    }

    /// @{
    /// @brief Get the statements.
    /// @return the statements

    std::vector<std::shared_ptr<statement_v1>>& get_statements()
    {
        return m_statements;
    }

    const std::vector<std::shared_ptr<statement_v1>>& get_statements() const
    {
        return m_statements;
    }

    /// @}

    /// @brief The type of a constant iterator over the statements in a statement list.
    using const_iterator = typename std::vector<std::shared_ptr<statement_v1>>::const_iterator;
    
    /// @{
    /// @brief Return a constant iterator pointing to begin of the statement list.
    /// @return the iterator
    const_iterator cbegin() const
    { return m_statements.cbegin(); }
    
    const_iterator begin() const
    { return m_statements.begin(); }

    /// @}

    /// @{
    /// @brief Get a constant iterator pointing to the end of the statement list.
    /// @return the iterator
    const_iterator cend() const
    { return m_statements.cend(); }
    
    const_iterator end() const
    { return m_statements.end(); }

    /// @}

}; // class statement_list_v1

/// @brief An empty line statement.
class empty_line_v1 : public statement_v1
{
public:
    /// @brief Construct this empty line statement.
    /// @param location the location of this empty line statement
    empty_line_v1(const id::location& location) :
        statement_v1(statement_kind_v1::empty_line, location)
    {}
}; // class empty_line_v1

/// @brief A line statement.
class line_v1 : public statement_v1
{
private:
	/// @brief The words of the line statement.
    std::vector<std::shared_ptr<word_v1>> m_words;

public:
	/// @brief Construct this line statement.
    /// @param location the location of this line statement
	line_v1(const id::location& location) :
        statement_v1(statement_kind_v1::line, location)
	{}

    /// @brief Set the words.
    /// @param words the words
    void set_words(const std::vector<std::shared_ptr<word_v1>>& words)
    {
        m_words = words;
    }

    /// @{
    /// @brief Get the words.
    /// @return the words

    std::vector<std::shared_ptr<word_v1>>& get_words()
    {
        return m_words;
    }

    const std::vector<std::shared_ptr<word_v1>>& get_words() const
    {
        return m_words;
    }

    /// @}

}; // class line_v1

/// @brief A block statement.
class block_v1 : public statement_v1
{
private:
	/// @brief The statement list of this block.
	std::shared_ptr<statement_list_v1> m_statement_list;

public:
	/// @brief Construct this block statement.
    /// @param location the location of this block statement
	block_v1(const id::location& location) :
        statement_v1(statement_kind_v1::block, location)
	{}
	
	/// @brief Set the statements of this block.
	/// @param statements the statements
	void set_statement_list(const std::shared_ptr<statement_list_v1>& statement_list)
	{ m_statement_list = statement_list; }

	/// @{

	/// @brief Get the statement list of this block.
	/// @return the statement list
	std::shared_ptr<statement_list_v1>& get_statement_list()
	{ return m_statement_list; }
	
	const std::shared_ptr<statement_list_v1>& get_statement_list() const
	{ return m_statement_list; }
	
	/// @}
	
}; // class block_v1

/// @brief A program.
class program_v1 : public node_v1
{
private:
    /// @brief The lstatement list of this program.
    std::shared_ptr<statement_list_v1> m_statement_list;

public:
    /// @brief Construct this program.
    /// @param location the location of this program
    program_v1(const id::location& location)
        : node_v1(node_kind_v1::program, location),
          m_statement_list(std::make_shared<statement_list_v1>(location))
    {}

	/// @brief Set the statement list of this program.
	/// @param statement_list the statement list
	void set_statement_list(const std::shared_ptr<statement_list_v1>& statement_list)
	{ m_statement_list = statement_list; }

	/// @{
	/// @brief Get the statement list of this program.
	/// @return the statement list

	std::shared_ptr<statement_list_v1>& get_statement_list()
	{ return m_statement_list; }

	const std::shared_ptr<statement_list_v1>& get_statement_list() const
	{ return m_statement_list; }

	/// @}

}; // class program_v1
