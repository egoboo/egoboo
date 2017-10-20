#pragma once

#include "egolib/FileFormats/SpawnFile/SpawnFileTokenKind.hpp"

class SpawnFileToken : public id::c::token<SpawnFileTokenKind, SpawnFileTokenKind::Unknown>
{
public:
    SpawnFileToken
        (
            SpawnFileTokenKind kind,
            const id::c::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
