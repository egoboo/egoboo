#pragma once

#include "egolib/FileFormats/SpawnFile/SpawnFileTokenKind.hpp"

class SpawnFileToken : public Id::Token<SpawnFileTokenKind>
{
public:
    SpawnFileToken
        (
            SpawnFileTokenKind kind,
            const Id::Location& startLocation,
            const std::string& lexeme = std::string()
        );
};
