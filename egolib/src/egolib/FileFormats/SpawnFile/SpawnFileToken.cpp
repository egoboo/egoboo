#include "egolib/FileFormats/SpawnFile/SpawnFileToken.hpp"

SpawnFileToken::SpawnFileToken
    (
        SpawnFileTokenKind kind,
        const id::location& startLocation,
        const std::string& lexeme
    ) :
    id::token<SpawnFileTokenKind>(kind, startLocation, lexeme)
{}
