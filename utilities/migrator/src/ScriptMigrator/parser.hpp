#pragma once

#include "ScriptMigrator/scanner.hpp"

/// @brief A parser.
class parser
{
private:
    /// @brief A pointer to the underlying scanner.
    scanner *m_scanner;
public:
    /// @brief Construct this parser.
    /// @param scanner a pointer to the underlying lexical analysis
    /// @throw id::null_error @a scanner is null
    parser(scanner *scanner);

    /// @brief Destruct this parser.
    ~parser();

    /// @brief Run.
    void run();

}; // class parser