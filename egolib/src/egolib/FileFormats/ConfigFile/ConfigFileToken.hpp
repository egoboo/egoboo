#pragma once

#include "egolib/FileFormats/ConfigFile/ConfigFileTokenKind.hpp"

class ConfigFileToken : public id::token<ConfigFileTokenKind, ConfigFileTokenKind::Unknown>
{
public:
    ConfigFileToken
        (
            ConfigFileTokenKind kind,
            const id::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
