#include "egolib/FileFormats/SpawnFile/SpawnFileToken.hpp"

SpawnFileToken::SpawnFileToken
    (
        SpawnFileTokenKind kind,
        const Id::Location& startLocation,
        const std::string& lexeme
    ) :
    Token(kind, startLocation, lexeme)
{}