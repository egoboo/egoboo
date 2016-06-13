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

/// @file egolib/FileFormats/scancode_file.c
/// @brief Functions to read and write Egoboo's basicdat/scantag.txt file
/// @details

#include "egolib/FileFormats/scancode_file.h"

#include "egolib/Core/StringUtilities.hpp"

#include "egolib/Log/_Include.hpp"
#include "egolib/Input/input_device.h"

#include "egolib/vfs.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"

//--------------------------------------------------------------------------------------------

static const std::vector<scantag_t> scantag_lst{
    // key scancodes
    {"KEY_BACK_SPACE", SDLK_BACKSPACE},
    {"KEY_TAB", SDLK_TAB},
    {"KEY_RETURN", SDLK_RETURN},
    {"KEY_ESCAPE", SDLK_ESCAPE},
    {"KEY_SPACE", SDLK_SPACE},
    {"KEY_APOSTROPHE", SDLK_QUOTEDBL},
    {"KEY_GRAVE", SDLK_RIGHTPAREN},
    {"KEY_COMMA", SDLK_COMMA},
    {"KEY_MINUS", SDLK_MINUS},
    {"KEY_PERIOD", SDLK_PERIOD},
    {"KEY_SLASH", SDLK_SLASH},
    {"KEY_0", SDLK_0},
    {"KEY_1", SDLK_1},
    {"KEY_2", SDLK_2},
    {"KEY_3", SDLK_3},
    {"KEY_4", SDLK_4},
    {"KEY_5", SDLK_5},
    {"KEY_6", SDLK_6},
    {"KEY_7", SDLK_7},
    {"KEY_8", SDLK_8},
    {"KEY_9", SDLK_9},
    {"KEY_SEMICOLON", SDLK_SEMICOLON},
    {"KEY_EQUALS", SDLK_EQUALS},
    {"KEY_LEFT_BRACKET", SDLK_LEFTBRACKET},
    {"KEY_BACKSLASH", SDLK_BACKSLASH},
    {"KEY_RIGHT_BRACKET", SDLK_RIGHTBRACKET},
    {"KEY_A", SDLK_a},
    {"KEY_B", SDLK_b},
    {"KEY_C", SDLK_c},
    {"KEY_D", SDLK_d},
    {"KEY_E", SDLK_e},
    {"KEY_F", SDLK_f},
    {"KEY_G", SDLK_g},
    {"KEY_H", SDLK_h},
    {"KEY_I", SDLK_i},
    {"KEY_J", SDLK_j},
    {"KEY_K", SDLK_k},
    {"KEY_L", SDLK_l},
    {"KEY_M", SDLK_m},
    {"KEY_N", SDLK_n},
    {"KEY_O", SDLK_o},
    {"KEY_P", SDLK_p},
    {"KEY_Q", SDLK_q},
    {"KEY_R", SDLK_r},
    {"KEY_S", SDLK_s},
    {"KEY_T", SDLK_t},
    {"KEY_U", SDLK_u},
    {"KEY_V", SDLK_v},
    {"KEY_W", SDLK_w},
    {"KEY_X", SDLK_x},
    {"KEY_Y", SDLK_y},
    {"KEY_Z", SDLK_z},
    {"KEY_DELETE", SDLK_DELETE},
    {"KEY_PAD_0", SDLK_KP_0},
    {"KEY_PAD_1", SDLK_KP_1},
    {"KEY_PAD_2", SDLK_KP_2},
    {"KEY_PAD_3", SDLK_KP_3},
    {"KEY_PAD_4", SDLK_KP_4},
    {"KEY_PAD_5", SDLK_KP_5},
    {"KEY_PAD_6", SDLK_KP_6},
    {"KEY_PAD_7", SDLK_KP_7},
    {"KEY_PAD_8", SDLK_KP_8},
    {"KEY_PAD_9", SDLK_KP_9},
    {"KEY_PAD_PERIOD", SDLK_KP_PERIOD},
    {"KEY_PAD_SLASH", SDLK_KP_DIVIDE},
    {"KEY_PAD_ASTERISK", SDLK_KP_MULTIPLY},
    {"KEY_PAD_MINUS", SDLK_KP_MINUS},
    {"KEY_PAD_PLUS", SDLK_KP_PLUS},
    {"KEY_ENTER", SDLK_KP_ENTER},
    {"KEY_PAD_ENTER", SDLK_KP_ENTER},
    {"KEY_UP", SDLK_UP},
    {"KEY_DOWN", SDLK_DOWN},
    {"KEY_RIGHT", SDLK_RIGHT},
    {"KEY_LEFT", SDLK_LEFT},
    {"KEY_INSERT", SDLK_INSERT},
    {"KEY_HOME", SDLK_HOME},
    {"KEY_END", SDLK_END},
    {"KEY_PAGE_UP", SDLK_PAGEUP},
    {"KEY_PAGE_DOWN", SDLK_PAGEDOWN},
    {"KEY_F1", SDLK_F1},
    {"KEY_F2", SDLK_F2},
    {"KEY_F3", SDLK_F3},
    {"KEY_F4", SDLK_F4},
    {"KEY_F5", SDLK_F5},
    {"KEY_F6", SDLK_F6},
    {"KEY_F7", SDLK_F7},
    {"KEY_F8", SDLK_F8},
    {"KEY_F9", SDLK_F9},
    {"KEY_F10", SDLK_F10},
    {"KEY_F11", SDLK_F11},
    {"KEY_F12", SDLK_F12},
    {"KEY_F13", SDLK_F13},
    {"KEY_F14", SDLK_F14},
    {"KEY_F15", SDLK_F15},
    
    // key modifiers
    {"KEY_NUM_LOCK", SDLK_NUMLOCKCLEAR},
    {"KEY_CAPS_LOCK", SDLK_CAPSLOCK},
    {"KEY_SCROLL_LOCK", SDLK_SCROLLLOCK},
    {"KEY_RIGHT_SHIFT", SDLK_RSHIFT},
    {"KEY_LEFT_SHIFT", SDLK_LSHIFT},
    {"KEY_RIGHT_CONTROL", SDLK_RCTRL},
    {"KEY_LEFT_CONTROL", SDLK_LCTRL},
    {"KEY_RIGHT_ALT", SDLK_RALT},
    {"KEY_LEFT_ALT", SDLK_LALT},
    {"KEY_RIGHT_META", SDLK_RGUI},
    {"KEY_LEFT_META", SDLK_LGUI},
    {"KEY_RIGHT_SUPER", SDLK_LGUI},
    {"KEY_LEFT_SUPER", SDLK_RGUI},
    
    // mouse button bits
    {"MOS_LEFT", 1},
    {"MOS_RIGHT", 2},
    {"MOS_MIDDLE", 4},
    {"MOS_FOURTH", 8},
    {"MOS_LEFT_AND_RIGHT", 3},
    {"MOS_LEFT_AND_MIDDLE", 5},
    {"MOS_RIGHT_AND_MIDDLE", 6},
    
    // joy button bits
    {"JOY_0", 1 << 0},
    {"JOY_1", 1 << 1},
    {"JOY_2", 1 << 2},
    {"JOY_3", 1 << 3},
    {"JOY_4", 1 << 4},
    {"JOY_5", 1 << 5},
    {"JOY_6", 1 << 6},
    {"JOY_7", 1 << 7},
    {"JOY_8", 1 << 8},
    {"JOY_9", 1 << 9},
    {"JOY_10", 1 << 10},
    {"JOY_11", 1 << 11},
    {"JOY_12", 1 << 12},
    {"JOY_13", 1 << 13},
    {"JOY_14", 1 << 14},
    {"JOY_15", 1 << 15},
    {"JOY_16", 1 << 16},
    {"JOY_17", 1 << 17},
    {"JOY_18", 1 << 18},
    {"JOY_19", 1 << 19},
    {"JOY_20", 1 << 20},
    {"JOY_21", 1 << 21},
    {"JOY_22", 1 << 22},
    {"JOY_23", 1 << 23},
    {"JOY_24", 1 << 24},
    {"JOY_25", 1 << 25},
    {"JOY_26", 1 << 26},
    {"JOY_27", 1 << 27},
    {"JOY_28", 1 << 28},
    {"JOY_29", 1 << 29},
    {"JOY_30", 1 << 30},
    {"JOY_31", 1u << 31},
    {"JOY_0_AND_1", 3},
    {"JOY_0_AND_2", 5},
    {"JOY_0_AND_3", 9},
    {"JOY_1_AND_2", 6},
    {"JOY_1_AND_3", 10},
    {"JOY_2_AND_3", 12},
};

using scantag_const_iter = std::vector<scantag_t>::const_iterator;
using scantag_iter = std::vector<scantag_t>::iterator;

static scantag_const_iter scantag_find_bits(char device_char, uint32_t tag_bits,
                                            scantag_const_iter start = scantag_lst.cbegin());
static scantag_const_iter scantag_find_value(char device_char, uint32_t tag_value,
                                             scantag_const_iter start = scantag_lst.cbegin());
static const scantag_t *scantag_get_tag(int index);
static Uint32 scancode_get_kmod(Uint32 scancode);
static int scantag_find_index(const std::string& string);

//--------------------------------------------------------------------------------------------
Uint32 scancode_get_kmod(Uint32 scancode) {

    Uint32 kmod = 0;

    switch (scancode) {
        case SDLK_NUMLOCKCLEAR:  kmod = KMOD_NUM;    break;
        case SDLK_CAPSLOCK: kmod = KMOD_CAPS;   break;
        case SDLK_LSHIFT:   kmod = KMOD_LSHIFT; break;
        case SDLK_RSHIFT:   kmod = KMOD_RSHIFT; break;
        case SDLK_LCTRL:    kmod = KMOD_LCTRL;  break;
        case SDLK_RCTRL:    kmod = KMOD_RCTRL;  break;
        case SDLK_LALT:     kmod = KMOD_LALT;   break;
        case SDLK_RALT:     kmod = KMOD_RALT;   break;
        case SDLK_LGUI:     kmod = KMOD_LGUI;  break;
        case SDLK_RGUI:     kmod = KMOD_RGUI;  break;
        case SDLK_MODE:     kmod = KMOD_MODE;  break;

            // unhandled cases
        case SDLK_SCROLLLOCK:
        default:
            kmod = 0;
            break;
    }

    return kmod;
}

//--------------------------------------------------------------------------------------------
void scantag_parse_control(const std::string& tag_string, control_t &pcontrol) {
    static const std::string delimiters = " ,|+&\t\n";
    // Clear the control.
    pcontrol.clear();

    // do we have a valid string?
    if (tag_string.empty()) return;

    // Scan through the tag string for any valid commands,
    // terminate on any bad command.
    auto tokens = Ego::split(tag_string, delimiters);
    for (const auto &token : tokens) {
        if (delimiters.find(token) != std::string::npos) continue;

        int tag_index = scantag_find_index(token);
        if (tag_index < 0 || tag_index >= scantag_lst.size()) {
            if (token != "N/A") {
                Log::get().warn("%s - unknown tag token, \"%s\".\n", __FUNCTION__, token.c_str());
            }
            break;
        }

        auto tag_name = std::string(scantag_lst[tag_index].name);
        if (tag_name.empty()) continue;
        auto tag_value = scantag_lst[tag_index].value;

        if ('K' == tag_name[0]) {
            // Addd the tag value to the list of mapped keys.
            pcontrol.mappedKeys.push_front(tag_value);
            // Add the tag value to the modifier keys.
            pcontrol.tag_key_mods |= scancode_get_kmod(tag_value);
            // Mark the control as loaded.
            pcontrol.loaded = true;
        } else {
            pcontrol.tag_bits |= tag_value;

            pcontrol.loaded = true;
        }
    }
}

//--------------------------------------------------------------------------------------------
int scantag_find_index(const std::string& string) {
    /// @author ZZ
    /// @details Find the index of the scantag that matches the given string.
    ///    It will return -1 if there are no matches.

    // assume no matches
    int retval = -1;

    // find a match, if possible
    for (size_t cnt = 0; cnt < scantag_lst.size(); cnt++) {
        if (string == std::string(scantag_lst[cnt].name)) {
            // They match
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
static scantag_const_iter scantag_find_bits(char device_char, Uint32 tag_bits, scantag_const_iter start) {
    auto begin = scantag_lst.cbegin();
    auto end = scantag_lst.cend();
    if (start == end) {
        return start;
    }
    for (auto current = start; current != end; ++current) {
        if (device_char != (*current).name[0]) {
            continue;
        }
        if (HAS_ALL_BITS(tag_bits, (*current).value)) {
            return current;
        }
    }
    return end;
}

static scantag_const_iter scantag_find_value(char device_char, Uint32 tag_value, scantag_const_iter start) {
    auto begin = scantag_lst.cbegin();
    auto end = scantag_lst.cend();
    if (start == end) {
        return start;
    }
    for (auto current = start; current != end; ++current) {
        if (device_char != (*current).name[0]) {
            continue;
        }
        if (tag_value == (*current).value) {
            return current;
        }
    }
    return end;
}

//--------------------------------------------------------------------------------------------
std::string scantag_get_string(int device_type, const control_t &pcontrol) {
    /// @author ZF
    /// @details This translates a input pcontrol->tag value to a string

    // @todo Use a proper string buffer.
    std::string loc_buffer = "";

    // No tags yet.
    size_t tag_count = 0;

    // Get the device char.
    char device_char = get_device_char_from_device_type(device_type);

    // Find all the tags for the bits.
    if (pcontrol.tag_bits.any()) {
        unsigned long tag_bits = pcontrol.tag_bits.to_ulong();
        auto ptag = scantag_find_bits(device_char, tag_bits);
        while (scantag_lst.cend() != ptag) {
            // Get the tag name.
            auto tag_name = std::string(ptag->name);

            // Remove the tag bits from the control bits so that similar tag bits are not
            // counted multiple times.
            // Example: "joy_0 + joy_1" should not be decoded as "joy_0 + joy_1 + joy_0_and_1"
            tag_bits &= ~(ptag->value);

            // Do not append empty tag names.
            if (0 == tag_name.length()) {
                /* do nothing */
            }
            // If this is the first tag name.
            else if (0 == tag_count) {
                loc_buffer += tag_name;
                tag_count++;
            }
            // If this is not the first tag name.
            else {
                loc_buffer += " + ";
                loc_buffer += tag_name;
                tag_count++;
            }

            ptag = scantag_find_bits(device_char, tag_bits, ptag + 1);
        }
    }

    // Get the names for all the keyboard tags on this control
    // and put them in a list.
    if (!pcontrol.mappedKeys.empty()) {
        for (uint32_t keycode : pcontrol.mappedKeys) {
            auto ptag = scantag_find_value('K', keycode);
            if (scantag_lst.cend() == ptag) continue;

            // Get the tag name.
            auto tag_name = std::string(ptag->name);

            // Do not append empty tag names.
            if (0 == tag_name.length()) {
                /* do nothing */
            }
            // If this is the first tag name.
            else if (0 == tag_count) {
                loc_buffer += tag_name;
                tag_count++;
            }
            // If this is not the first tag name.
            else {
                loc_buffer += " + ";
                loc_buffer += tag_name;
                tag_count++;
            }
        }
    }
    if (0 == tag_count) {
        loc_buffer += "N/A";
    }
    return loc_buffer;
}
