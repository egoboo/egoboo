#include "egolib/FileFormats/ConfigFile/ConfigFileToken.hpp"

ConfigFileToken::ConfigFileToken
    (
        ConfigFileTokenKind kind,
        const Id::Location& startLocation,
        const std::string& lexeme
    ) :
    Token(kind, startLocation, lexeme)
{}