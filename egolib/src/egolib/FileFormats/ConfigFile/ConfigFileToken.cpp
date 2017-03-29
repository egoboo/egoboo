#include "egolib/FileFormats/ConfigFile/ConfigFileToken.hpp"

ConfigFileToken::ConfigFileToken
    (
        ConfigFileTokenKind kind,
        const id::location& startLocation,
        const std::string& lexeme
    ) :
    id::token<ConfigFileTokenKind>(kind, startLocation, lexeme)
{}