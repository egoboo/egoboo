#include "egolib/FileFormats/ConfigFile/ConfigFileToken.hpp"

ConfigFileToken::ConfigFileToken
    (
        ConfigFileTokenKind kind,
        const id::c::location& startLocation,
        const std::string& lexeme
    ) :
    id::c::token<ConfigFileTokenKind, ConfigFileTokenKind::Unknown>(kind, startLocation, lexeme)
{}