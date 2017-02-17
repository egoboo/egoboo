#pragma once

#include "IdLib/IdLib.hpp"

/// @brief The kinds of tokens in a configuration file.
enum class ConfigFileTokenKind
{
    /// @brief A name.
    Name,
    /// @brief A qualified name.
    QualifiedName,
    /// @brief A comment.
    Comment,
    /// @brief A colon.
    Colon,
};