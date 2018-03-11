#include "egolib/FileFormats/SpawnFile/SpawnFileToken.hpp"

SpawnFileToken::SpawnFileToken
    (
        SpawnFileTokenKind kind,
        const idlib::hll::location& startLocation,
        const std::string& lexeme
    ) :
    idlib::hll::token<SpawnFileTokenKind, SpawnFileTokenKind::Unknown>(kind, startLocation, lexeme)
{}
