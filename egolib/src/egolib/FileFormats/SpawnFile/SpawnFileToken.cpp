#include "egolib/FileFormats/SpawnFile/SpawnFileToken.hpp"

SpawnFileToken::SpawnFileToken
    (
        SpawnFileTokenKind kind,
        const id::c::location& startLocation,
        const std::string& lexeme
    ) :
    id::c::token<SpawnFileTokenKind, SpawnFileTokenKind::Unknown>(kind, startLocation, lexeme)
{}
