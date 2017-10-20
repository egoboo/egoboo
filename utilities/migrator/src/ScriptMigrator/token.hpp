#pragma once

#include "idlib/idlib.hpp"

/// @brief The kinds of the tokens.
enum class token_kind
{
    /// @brief An error.
    error = 0,
    /// @brief Start of input.
    start_of_input,
    /// @brief An indent.
    /// The text is string representing non-negative integer indicating the indention level.
    indent,
    /// @brief A line.
    /// The text is a string representing the line.
    line,
    /// @brief A comment.
    comment,
    /// @brief End of input.
    end_of_input,
}; // enum class token_kind

/// @brief A token.
using token = id::c::token<token_kind, token_kind::error>;

static_assert(std::is_nothrow_move_constructible<token>::value, "token should be nothrow move constructible");
#if !defined(__GNUC__) || __GNUC__ > 6
static_assert(std::is_nothrow_move_assignable<token>::value, "token should be nothrow move assignable");
#endif
// As token is using std::string, this will not hold.
//static_assert(std::is_nothrow_copy_constructible<token>::value, "token should be nothrow copy contructible");
// As token is using std::string, this will not hold.
//static_assert(std::is_nothrow_copy_assignable<token>::value, "token should be nothrow copy assignable");