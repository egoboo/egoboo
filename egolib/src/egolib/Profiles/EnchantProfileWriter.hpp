//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file  egolib/Profiles/EnchantProfileWriter.hpp
/// @brief Writes Egoboo's enchant profile files (<tt>"/modules/*.mod/objects/*.obj/enchant.txt"</tt>).


#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/Profiles/EnchantProfile.hpp"

/**
 * @brief
 *  A writer for enchantment profiles.
 */
struct EnchantProfileWriter
{
    /**
     * @brief
     *  Write an enchantment profile.
     * @param profile
     *  the enchantment profile from which the data to write is stored in
     * @param pathname
     *  the pathname of the file to write the data to
     * @param templateName
     *  the template name
     * @return
     *  @a true on success, @a false on failure
     */
    static bool write(std::shared_ptr<EnchantProfile> profile, const std::string& pathname, const char *templateName);
};
