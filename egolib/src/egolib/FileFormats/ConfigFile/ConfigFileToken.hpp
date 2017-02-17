#pragma once

#include "egolib/FileFormats/ConfigFile/ConfigFileTokenKind.hpp"

class ConfigFileToken : public Id::Token<ConfigFileTokenKind>
{
public:
    ConfigFileToken
        (
            ConfigFileTokenKind kind,
            const Id::Location& startLocation,
            const std::string& lexeme = std::string()
        );
};
