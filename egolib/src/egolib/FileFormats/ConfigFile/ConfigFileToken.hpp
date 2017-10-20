#pragma once

#include "egolib/FileFormats/ConfigFile/ConfigFileTokenKind.hpp"

class ConfigFileToken : public id::c::token<ConfigFileTokenKind, ConfigFileTokenKind::Unknown>
{
public:
    ConfigFileToken
        (
            ConfigFileTokenKind kind,
            const id::c::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
