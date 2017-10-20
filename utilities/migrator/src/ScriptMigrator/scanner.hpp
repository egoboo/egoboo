#pragma once

#include "ScriptMigrator/token.hpp"

/// @brief A scanner.
class scanner
{
private:
    /// @brief A pointer to the address before the first Byte of the input.
    const char *m_start;

    /// @brief A pointer to the address behind of the last Byte of the input.
    const char *m_end;

    /// @brief A pointer to the address before the current Byte of the input.
    const char *m_current;

    /// @brief The source location before the first Byte of the input
    id::c::location m_location;

    /// @brief The lexeme buffer.
    std::vector<char> m_buffer;

public:
    /// @brief Construct this scanner.
    /// @param location the source location before the first Byte of the input
    /// @param start a pointer to the address before of the first Byte of the input
    /// @param end a pointer to the address behind the last Byte of the input
    /// @param current a pointer to the address before the current Byte of the input
    /// @throw id::null_error @a start is null
    /// @throw id::null_error @a end is null
    scanner(const id::c::location& location, const char *start, const char *end, const char *current);

    /// @brief Destruct this scanner.
    ~scanner();

    /// @brief Run.
    std::vector<token> run();

}; // class scanner