#include "egolib/FileFormats/SpawnFile/SpawnFileToken.hpp"

SpawnFileToken::SpawnFileToken
    (
        SpawnFileTokenKind kind,
        const idlib::c::location& startLocation,
        const std::string& lexeme
    ) :
    idlib::c::token<SpawnFileTokenKind, SpawnFileTokenKind::Unknown>(kind, startLocation, lexeme)
{}
