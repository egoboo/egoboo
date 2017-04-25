#pragma once

#include <regex>

/// @brief A regular expression filter using the ECMAScript variant optimizing
/// the regular expression. This means: create once and re-use often for best
/// performance.
struct RegexFilter
{
private:
    std::regex _regex;
public:
    RegexFilter(const std::string& regexString) :
        _regex(regexString, std::regex_constants::ECMAScript | std::regex_constants::optimize)
    {}
public:
    bool operator()(const std::string &s) const
    {
        return std::regex_match(s, _regex);
    }
};
