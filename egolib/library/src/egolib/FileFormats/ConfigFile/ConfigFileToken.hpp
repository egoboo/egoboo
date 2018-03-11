#pragma once

#include "egolib/FileFormats/ConfigFile/ConfigFileTokenKind.hpp"
#include "idlib/hll.hpp"

class ConfigFileToken : public idlib::hll::token<ConfigFileTokenKind, ConfigFileTokenKind::Unknown>
{
public:
    ConfigFileToken
        (
            ConfigFileTokenKind kind,
            const idlib::hll::location& startLocation,
            const std::string& lexeme = std::string()
        );
};
