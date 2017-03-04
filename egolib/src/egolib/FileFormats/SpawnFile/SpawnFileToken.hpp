#pragma once

#include "egolib/FileFormats/SpawnFile/SpawnFileTokenKind.hpp"

class SpawnFileToken : public Id::Token<SpawnFileTokenKind>
{
public:
    SpawnFileToken
        (
            SpawnFileTokenKind kind,
            const id::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
