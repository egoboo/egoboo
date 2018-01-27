#pragma once

#include <string>
#include <regex>

namespace idlib { namespace file_system { namespace internal {

template <typename C>
struct path_parser
{
	std::basic_string<C> operator()(const std::basic_string<C>& w) const
	{ 
        static const std::basic_regex<C> r("(/|\\\\)+");
        return std::regex_replace(w, r, "/");
    }
}; // struct path_parser

} } } // namespace idlib::file_system::internal
