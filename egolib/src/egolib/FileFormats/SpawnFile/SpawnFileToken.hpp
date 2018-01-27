#pragma once

#include "egolib/FileFormats/SpawnFile/SpawnFileTokenKind.hpp"

class SpawnFileToken : public idlib::c::token<SpawnFileTokenKind, SpawnFileTokenKind::Unknown>
{
public:
    SpawnFileToken
        (
            SpawnFileTokenKind kind,
            const idlib::c::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
