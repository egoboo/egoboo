#pragma once

#include "idlib/utility.hpp"
#include "ScriptMigrator/tree_v1_forward.hpp"

/// @brief An unparser.
class unparser_v1
{
    size_t m_indent;
    std::function<void(char)> m_write;
    template <typename Iterator>
    void write(Iterator begin, Iterator end)
    {
        for (auto it = begin; it != end; ++it)
        {
            write(*it);
        }
    }
    void write(char x)
    { 
        m_write(x); 
    }
public:
    /// @brief Construct this unparser.
    unparser_v1();

    /// @brief Destruct this unparser.
    ~unparser_v1();

protected:
    void word(const std::shared_ptr<::word_v1>& word);
    void empty_line(const std::shared_ptr<::empty_line_v1>& empty_line);
    void line(const std::shared_ptr<::line_v1>& line);
    void statement(const std::shared_ptr<::statement_v1>& statement);
    void statement_list(const std::shared_ptr<::statement_list_v1>& statement_list);
    void block(const std::shared_ptr<::block_v1>& block);
    void program(const std::shared_ptr<::program_v1>& program);

    /// Remove all empty lines at the end of the statements of a block.
    /// @param block the black
    void remove_empty_lines_at_end_of_block(const std::shared_ptr<::block_v1>& block);

public:
    /// @brief Run this unparser.
    /// @param program a shared pointer to a program
    /// @param write a function which receives the Bytes from the unparser
    void run(const std::shared_ptr<::program_v1>& program,
             std::function<void(char)> write);

}; // class unparser_v1
