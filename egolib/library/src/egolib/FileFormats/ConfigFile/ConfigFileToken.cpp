#include "egolib/FileFormats/ConfigFile/ConfigFileToken.hpp"

ConfigFileToken::ConfigFileToken
    (
        ConfigFileTokenKind kind,
        const idlib::hll::location& startLocation,
        const std::string& lexeme
    ) :
    idlib::hll::token<ConfigFileTokenKind, ConfigFileTokenKind::Unknown>(kind, startLocation, lexeme)
{}
