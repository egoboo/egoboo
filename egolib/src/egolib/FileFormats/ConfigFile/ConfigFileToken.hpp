#pragma once

#include "egolib/FileFormats/ConfigFile/ConfigFileTokenKind.hpp"

class ConfigFileToken : public idlib::c::token<ConfigFileTokenKind, ConfigFileTokenKind::Unknown>
{
public:
    ConfigFileToken
        (
            ConfigFileTokenKind kind,
            const idlib::c::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
