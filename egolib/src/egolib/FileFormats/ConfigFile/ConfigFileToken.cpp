#include "egolib/FileFormats/ConfigFile/ConfigFileToken.hpp"

ConfigFileToken::ConfigFileToken
    (
        ConfigFileTokenKind kind,
        const idlib::c::location& startLocation,
        const std::string& lexeme
    ) :
    idlib::c::token<ConfigFileTokenKind, ConfigFileTokenKind::Unknown>(kind, startLocation, lexeme)
{}