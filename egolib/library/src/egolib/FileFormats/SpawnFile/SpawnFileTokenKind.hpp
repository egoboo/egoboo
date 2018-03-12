#pragma once

/// @brief The kinds of tokens in a spawn file.
enum class SpawnFileTokenKind
{
    /// @brief Unknown.
    Unknown = 0,
    /// @brief Start of input.
    StartOfInput,
    /// @brief A commentary.
    Commentary,
    /// @brief A dependency.
    /// @code
    /// '#dependency'
    /// @endcode
    Dependency,
    /// @brief A name.
    Name,
    /// @brief A natural number literal.
    Natural,
};
