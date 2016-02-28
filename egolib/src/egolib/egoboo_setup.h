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

/// @file  egolib/egoboo_setup.h
/// @brief Functions for handling the <tt>setup.txt</tt> file.

#pragma once

#include "egolib/typedef.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/Script/EnumDescriptor.hpp"
#include "egolib/FileFormats/configfile.h"
#include "egolib/Script/Conversion.hpp"
#include "egolib/Renderer/TextureFilter.hpp"

//Forward declarations
enum class CameraTurnMode : uint8_t;
struct egoboo_config_t;

//--------------------------------------------------------------------------------------------
// CONSTANTS
//--------------------------------------------------------------------------------------------

namespace Ego
{
    // The game difficulties.
    enum class GameDifficulty
    {
        /// Easy.
        Easy = 0,
        /// Normal.
        Normal,
        /// Hard.
        Hard,
    };
}

//--------------------------------------------------------------------------------------------
namespace Ego
{
    // Different types of feedback.
    enum class FeedbackType
    {
        // Show no feedback.
        None = 0,
        // Show a descriptive text.
        Text,
        // Show damage as numbers.
        Number,
    };

}

namespace Ego
{
namespace Configuration
{

using namespace std;

template <typename ValueType>
class Variable : public Id::NonCopyable
{

private:

    /**
     * @brief
     *  The default value of the variable.
     */
    const ValueType _defaultValue;

    /**
     * @brief
     *  The partially qualified name of this element e.g. <tt>video.fullscreen</tt>.
     */
    string _name;

    /**
     * @brief
     *  The description of the variable e.g. <tt>Enable/disable fullscreen mode.</tt>.
     */
    string _description;

    /**
     * @brief
     *  The value of the variable.
     */
    ValueType _value;

protected:

    /**
     * @brief
     * @param defaultValue
     *  the default value
     * @param qualifiedName
     *  the qualified name of the variable
     * @param description
     *  the description of the variable
     */
    Variable(const ValueType& defaultValue, const string& name, const string& description) :
        _defaultValue(defaultValue), _name(name), _description(description),
        _value(defaultValue)
    {}
            
    /**
     * @brief
     *  Destruct this variable.
     */
    virtual ~Variable()
    {}

public:

    /**
     * @brief
     *  Get the value of this variable.
     * @return
     *  the value of the variable
     */
    const ValueType& getValue() const
    {
        return _value;
    }

    /**
     * @brief
     *  Set the value of this variable.
     * @param value
     *  the value
     */
    void setValue(const ValueType& value)
    {
        _value = value;
    }

    /**
     * @brief
     *  Get the default value of the variable.
     * @return
     *  the default value of this variable
     */
    const ValueType& getDefaultValue() const
    {
        return _defaultValue;
    }

    /**
     * @brief
     *  Get the qualified name of this variable.
     * @return
     *  the qualified name of this variable
     */
    const string getName() const
    {
        return _name;
    }

    /**
     * @brief
     *  Get the description of this variable.
     * @return
     *  the description of this variable
     */
    const string getDescription() const
    {
        return _description;
    }

    /**
     * @brief
     *  Encode and store this element's value into to a string.
     * @param target
     *  the target string
     * @return
     *  @a true on success, @a false on failure
     */
    virtual bool encodeValue(string& target) const = 0;

    /**
     * @brief
     *  Load and decode this element's value from a string.
     * @param source
     *  the source string
     * @return
     *  @a true on success, @a false on failure
     */
    virtual bool decodeValue(const string& source) = 0;
};

template <class ValueType>
class NumericVariable : public Variable<ValueType>
{
    // (u)intx_t, x in [8,16,32,64] & float & double
    static_assert(!is_same<ValueType, bool>::value && is_arithmetic<ValueType>::value, "ValueType must not be an arithmetic non-bool type");

private:

    /**
     * @brief
     *  The minimum value (inclusive).
     * @invariant
     *   <tt>_min <= _max</tt>
     */
    ValueType _min;
    /**
     * @brief
     *  The maximum value (inclusive).
     * @invariant
     *  <tt>_min <= _max</tt>
     */
    ValueType _max;
public:
    /**
     * @brief
     * @param defaultValue
     *  the default value the variable
     * @param name
     *  the partially qualified name of the variable
     * @param description
     *  the description of the variable
     * @throw std::invalid_argument
     *  if <tt>min > max</tt> or <tt>defaultValue < min</tt> or <tt>defaultValue > max</tt>
     */
    NumericVariable(const ValueType& defaultValue, const string& name, const string& description,
                    const ValueType& min, const ValueType& max) :
        Variable<ValueType>(defaultValue, name, description), _min(min), _max(max)
    {
        if (min > max) throw std::invalid_argument("min > max");
        else if (defaultValue < min) throw std::invalid_argument("defaultValue < min");
        else if (defaultValue > max) throw std::invalid_argument("defaultValue > max");
    }

    const NumericVariable& operator=(const NumericVariable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override
    {
        return Ego::Script::Encoder<ValueType>()(this->getValue(), target);
    }

    virtual bool decodeValue(const string& source) override
    {
        ValueType temporary;
        if (!Ego::Script::Decoder<ValueType>()(source, temporary))
        {
            return false;
        }
        this->setValue(temporary);
        return true;
    }

    ValueType getMaxValue() const { return _max; }

    ValueType getMinValue() const { return _min; }
};

/**
 * @brief
 *  An element of a configuration with a "name" and a "desciption".
 * @remark
 *  An extra specialization of @a ValueType is an enumeration type is provided.
 */
template <class ValueType>
class StandardVariable : public Variable<ValueType>
{
    static_assert(!is_enum<ValueType>::value, "ValueType must not be an enumeration type");

public:

    /**
     * @brief
     * @param defaultValue
     *  the default value
     * @param name
     *  the qualified name of the variable
     * @param description
     *  the description of the variable
     */
    StandardVariable(const ValueType& defaultValue, const string& name, const string& description) :
        Variable<ValueType>(defaultValue, name, description)
    {}

    StandardVariable& operator=(const StandardVariable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override
    {
        return Ego::Script::Encoder<ValueType>()(this->getValue(), target);
    }

    virtual bool decodeValue(const string& source) override
    {
        ValueType temporary;
        if (!Ego::Script::Decoder<ValueType>()(source, temporary))
        {
            return false;
        }
        this->setValue(temporary);
        return true;
    }

};

/**
 * @brief
 *  An element of a configuration with a "name" and a "desciption".
 * @remark
 *  We can't use a single variable template and use SFINAE to specialize for
 *  certain types (i.e. for enumerations, for integral types, etc.) because
 *  Redmond Retards don't support SFINAE yet - in fact MSVC crashes upon
 *  <tt>enable_if<is_enum<ValueType>::value>::type</tt>.
 *  See
 *  - http://en.cppreference.com/w/cpp/types/is_enum
 *  - http://en.cppreference.com/w/cpp/types/enable_if and
 *  - http://en.cppreference.com/w/cpp/language/sfinae
 *  for more information.
 */
template <typename ValueType>
class EnumVariable : public Variable<ValueType>
{

private:

    // Static checking template arguments.
    static_assert(is_enum<ValueType>::value, "value type must be an enumeration");

    /**
        * @brief
        *  The descriptor of the enumeration.
        */
    Ego::Script::EnumDescriptor<ValueType> _enumDescriptor;

public:

    /**
     * @brief
     * @param name
     *  the partially qualified name of the variable
     * @param description
     *  the description of the variable
     */
    EnumVariable(const ValueType& defaultValue, const string& name, const string& description, const initializer_list<pair<const string, ValueType>> list) :
        Variable<ValueType>(defaultValue, name, description), _enumDescriptor(name, list)
    {}

    EnumVariable& operator=(const EnumVariable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override
    {
        auto it = _enumDescriptor.find(this->getValue());
        if (it == _enumDescriptor.end())
        {
            return false;
        }
        target = it->first;
        return true;
    }

    virtual bool decodeValue(const string& source) override
    {
        auto it = _enumDescriptor.find(source);
        if (it == _enumDescriptor.end())
        {
            return false;
        }
        this->setValue(it->second);
        return true;
    }
};

} // Script
} // Ego

//--------------------------------------------------------------------------------------------
// struct egoboo_config_t
//--------------------------------------------------------------------------------------------

using namespace Ego::Configuration;

/// The internal representation of the data in "settings.txt"
/**
 * @remark
 *  The suffix "_max" usually indicates that the variable provides an inclusive upper bound
 *  of a property, the corresponding suffix "_min" that it provides an inclusive lower bound.
 *  Variables with the "_count", "_size" suffix provide a desired/required value of a countable
 *  or measurable property. For binary choices which desire/require that something is
 *  enabled/disabled, the suffix "_enable" is used.
 */
struct egoboo_config_t
{
protected:

    static egoboo_config_t _singleton;

    // This is utility code for iterating over an std::tuple.
    template<typename TupleType, typename FunctionType>
    void for_each(TupleType&&, FunctionType, std::integral_constant<size_t,
        std::tuple_size<typename std::remove_reference<TupleType>::type >::value>)
    {
    }

    // This is utility code for iterating over an std::tuple.
    template<std::size_t I, typename TupleType, typename FunctionType,
        typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
        void for_each(TupleType&& t, FunctionType f, std::integral_constant<size_t, I>)
    {
        f(std::get<I>(t)); // Call the function.
        for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, I + 1>()); // Advance.
    }

public:

    // This is utility code for iterating over an std::tuple.
    template<typename TupleType, typename FunctionType>
    void for_each(TupleType&& t, FunctionType f)
    {
        for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
    }

    struct Load
    {
    private:
        shared_ptr<ConfigFile> _source;
    public:
        Load(const Load& other) :
            _source(other._source)
        {}
        Load& operator=(const Load& other)
        {
            _source = other._source;
            return *this;
        }
        Load(const shared_ptr<ConfigFile> &source) :
            _source(source)
        {}
        template<typename VariableTy>
        void operator()(VariableTy& variable) const
        {
            string valueString;
            if (!_source->get(variable.getName(), valueString))
            {
                variable.setValue(variable.getDefaultValue());
            }
            else
            {
                variable.decodeValue(valueString);
            }
        }
    };

    struct Store
    {
    private:
        shared_ptr<ConfigFile> _target;
    public:
        Store(const Store& other) :
            _target(other._target)
        {}
        Store(const shared_ptr<ConfigFile> &target) :
            _target(target)
        {}
        Store& operator=(const Store& other)
        {
            _target = other._target;
            return *this;
        }
        template<typename Variable>
        void operator()(Variable& variable) const
        {
            string value;
            variable.encodeValue(value);
            _target->set(variable.getName(), value);
        }
    };

    template <typename Functor>
    void for_each(Functor f)
    {
        auto variables =
            std::tie
            (
            graphic_fullscreen,
            graphic_colorBuffer_bitDepth,
            graphic_depthBuffer_bitDepth,
            graphic_resolution_horizontal,
            graphic_resolution_vertical,
            graphic_perspectiveCorrection_enable,
            graphic_dithering_enable,
            graphic_reflections_enable,
            graphic_reflections_particleReflections_enable,
            graphic_shadows_enable,
            graphic_shadows_highQuality_enable,
            graphic_overlay_enable,
            graphic_specularHighlights_enable,
            graphic_twoLayerWater_enable,
            graphic_background_enable,
            graphic_fog_enable,
            graphic_gouraudShading_enable,
            graphic_antialiasing,
            graphic_anisotropy_enable,
            graphic_anisotropy_levels,
            graphic_textureFilter_minFilter,
            graphic_textureFilter_magFilter,
            graphic_textureFilter_mipMapFilter,
            graphic_simultaneousDynamicLights_max,
            graphic_framesPerSecond_max,
            graphic_simultaneousParticles_max,
            graphic_hd_textures_enable,
            //
            sound_effects_enable,
            sound_effects_volume,
            sound_music_enable,
            sound_music_volume,
            sound_channel_count,
            sound_outputBuffer_size,
            sound_highQuality_enable,
            sound_footfallEffects_enable,
            //
            network_enable,
            network_lagTolerance,
            network_hostName,
            network_playerName,
            //
            game_difficulty,
            //
            camera_control,
            //
            hud_feedback,
            hud_simultaneousMessages_max,
            hud_messageDuration,
            hud_messages_enable,
            hud_displayStatusBars,
            hud_displayGameTime,
            hud_displayFramesPerSecond,
            //
			debug_mesh_renderHeightMap,
			debug_mesh_renderNormals,
            debug_hideMouse,
            debug_grabMouse,
            debug_developerMode_enable,
            debug_sdlImage_enable
            );
        for_each(variables, f);
    }

public:

    // Graphic configuration section.

    /**
     * @brief
     *  Enable/disable fullscreen mode.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_fullscreen;

    /**
     * @brief
     *  The horizontal resolution.
     * @remark
     *  Default value is @a 800.
     * @todo
     *  Type should be <tt>uint16_t</tt>.
     */
    StandardVariable<int> graphic_resolution_horizontal;

    /**
     * @brief
     *  The vertical resolution.
     * @remark
     *  Default value is @a 600.
     * @todo
     *  Type should be <tt>uint16_t</tt>.
     */
    StandardVariable<int> graphic_resolution_vertical;

    /**
     * @brief
     *  The color buffer depth.
     * @remark
     *  Default value is @a 24.
     * @todo
     *  Type should be <tt>uint8_t</tt>.
     */
    StandardVariable<int> graphic_colorBuffer_bitDepth;

    /**
     * @brief
     *  The depth buffer depth.
     * @remark
     *  Default value is @a 8.
     * @todo
     *  Type should be <tt>uint8_t</tt>.
     */
    StandardVariable<int> graphic_depthBuffer_bitDepth;

    /**
     * @brief
     *  Enable perspective correction.
     * @remark
     *  Default value is @a false.
     * @todo
     *  Enable this by default?
     */
    StandardVariable<bool> graphic_perspectiveCorrection_enable;

    /**
     * @brief
     *  Enable dithering.
     * @remark
     *  Default value is @a false.
     * @todo
     *  Enable this by default?
     */
    StandardVariable<bool> graphic_dithering_enable;

    /**
     * @brief
     *  Enable/disable reflections.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_reflections_enable;

    /**
     * @brief
     *  Enable/disable particle reflections.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_reflections_particleReflections_enable;

    /**
     * @brief
     *  Enable/disable shadows.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_shadows_enable;

    /**
     * @brief
     *  Enable/disable high-quality shadows.
     * @remark
     *  Default value is @a true (high quality).
     */
    StandardVariable<bool> graphic_shadows_highQuality_enable;

    /**
     * @brief
     *  Enable/disable specular highlights.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_specularHighlights_enable;

    /**
     * @brief
     *  Enable/disable two layer water.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_twoLayerWater_enable;

    /**
     * @brief
     *  Enable/disable overlay.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_overlay_enable;

    /**
     * @brief
     *  Enable/disable background.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_background_enable;

    /**
     * @brief
     *  Enable/disable fog.
     * @remark
     *  Default value is @a false.
     */
    StandardVariable<bool> graphic_fog_enable;

    /**
     * @brief
     *  Enable/disable Gouraud shading.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> graphic_gouraudShading_enable;

    /**
     * @brief
     *  Anti-aliasing level.
     * @remark
     *  Default is @ 2 i.e.two samples, the default value range is 0 (disabled), 2 (2x), 4 (4x) and 8 (8x) and 16 (16x).
     * @remark
     *  "multisampling" is a specific optimization of "super sampling".
     *  "super sampling anti-aliasing (SSAA)" and "full scene antialising (FSAA)" are synonyms.
     *  In general SSAA/FSAA is expensive and is replaced by by "multisample antialiasing (MSAA)".
     */
    StandardVariable<uint8_t> graphic_antialiasing;

    /**
     * @brief
     *  Enable anisotropy.
     * @remark
     *  Default is @a false.
     */
    StandardVariable<bool> graphic_anisotropy_enable;

    /**
     * @brief
     *  The anisotropy levels.
     * @remark
     *  Default is @a 1 within the range of <tt>[1,16]</tt>
     */
    NumericVariable<float> graphic_anisotropy_levels;

    /**
     * @brief
     *  The texture filter used for minification.
     * @remark
     *  Default value is Ego::TextureFilter::Linear.
     */
    EnumVariable<Ego::TextureFilter> graphic_textureFilter_minFilter;
    /**
     * @brief
     *  The texture filter used for magnification.
     * @remark
     *  Default value is Ego::TextureFilter::Linear.
     */
    EnumVariable<Ego::TextureFilter> graphic_textureFilter_magFilter;
    /**
     * @brief
     *  The filter applied used for mip map selection.
     * @remark
     *  Default value is Ego::TextureFilter::Linear.
     */
    EnumVariable<Ego::TextureFilter> graphic_textureFilter_mipMapFilter;

    /**
     * @brief
     *  Inclusive upper bound of number of simultaneous dynamic lights.
     * @remark
     *  Default value is @a 32.
     */
    StandardVariable<uint16_t> graphic_simultaneousDynamicLights_max;
    
    /**
     * @brief
     *  Inclusive upper bound of the frames per second.
     * @remark
     *  Default value is @a 30.
     */
    StandardVariable<uint16_t> graphic_framesPerSecond_max;
        
    /**
     * @brief
     *  Inclusive upper bound of the number of simultaneous particles.
     * @remark
     *  Default value is @a 768.
     */
    StandardVariable<uint16_t> graphic_simultaneousParticles_max;


    /**
    * @brief
    *   If true, the game will try to load HD versions of textures if
    *   they are available and default back to normal version if not.
    *   HD textures are textures with higher resolution and end with
    *   _HD before the file type suffix (e.g myTexture_HD.png).
    * @remark
    *   Default value is @a true.
    **/
    StandardVariable<bool> graphic_hd_textures_enable;

    // Sound configuration section.

    /**
     * @brief
     *  Enable/disable effects.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> sound_effects_enable;

    /**
     * @brief
     *  The effects volume.
     * @remark
     *  The default value is @a 75, range is @a 0 to @a 128.
     * @todo
     *  The unit should be "percent", the default value should be 60 "percent"?
     *  We require a "percent" type?
     */
    StandardVariable<Uint8> sound_effects_volume;

    /**
     * @brief
     *  Enable/disable music.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> sound_music_enable;

    /**
     * @brief
     *  The music volume.
     * @remark
     *  The default value is @a 50, range is @a 0 to @a 128.
     * @todo
     *  The unit should be "percent", the default value should be 40 "percent"?
     *  We require a "percent" type?
     */
    StandardVariable<Uint8> sound_music_volume;

    /**
     * @brief
     *  Number of audio channels.
     * @rma
     * @remark
     *  The number of audio channels limit the number of sounds playing at the same time.
     *  The default value is @a 32.
     * @todo
     *  Encode the following constraints
     *  @code
     *  CLIP<uint16_t>(cfg.sound_channel_count, 8, 128);
     *  CLIP<uint16_t>(cfg.sound_buffer_size, 512, 8196);
     *  @endcode
     */
    StandardVariable<Uint16> sound_channel_count;

    /**
     * @brief
     *  Size of audio output buffer.
     * @remark
     *  Default value is @a 4096.
     * @todo
     *  Encode the following constraints
     *  @code
     *  CLIP<uint16_t>(cfg.sound_buffer_size, 512, 8196);
     *  @endcode
     */
    StandardVariable<Uint16> sound_outputBuffer_size;

    /**
     * @brief
     *  Enable/disable high quality audio.
     * @remark
     *  Default value is @a false.
     */
    StandardVariable<bool> sound_highQuality_enable;

    /**
     * @brief
     *  Enable/disable footfall effects.
     * @remark
     */
    StandardVariable<bool> sound_footfallEffects_enable;

    // Network configuration section.

    /**
     * @brief
     *  Enable/disable network?
     * @remark
     *  Default value is @a false.
     */
    StandardVariable<bool> network_enable;

    /**
     * @brief
     *  Tolerance to lag.
     * @remark
     *  Default value is @a 10.
     * @todo
     *  And what does @a 10 mean?
     */
    StandardVariable<uint16_t> network_lagTolerance;

    /**
     * @brief
     *  Name of host to join.
     * @remark
     *  Default value is @a "Egoboo Host".
     */
    StandardVariable<std::string> network_hostName;

    /**
     * @brief
     *  Player name in network games.
     * @remark
     *  Default value is @a "Egoboo Player".
     */
    StandardVariable<std::string> network_playerName;

    // Camera configuration section.

    /**
     * @brief
     *  Type of camera control.
     * @remark
     *  Default value is @a CameraTurnMode::Auto.
     */
    EnumVariable<CameraTurnMode> camera_control;

    // Game configuration section.

    /**
     * @brief
     *  Game difficulty.
     * @remark
     *  Default value is Ego::GameDifficulty::Normal.
     */
    EnumVariable<Ego::GameDifficulty> game_difficulty;

    // HUD configuration section.

    /**
     * @brief
     *  Inclusive upper bound of simultaneous messages.
     * @remark
     *  Default value is @a 6.
     * @todo
     *  Encode the following constraint:
     *  @code
     *  CLIP(cfg->hud_simultaneousMessages_max.getValue(), (uint8_t)EGO_MESSAGE_MIN, (uint8_t)EGO_MESSAGE_MAX);
     *  @endcode
     */
    StandardVariable<uint8_t> hud_simultaneousMessages_max;

    /**
     * @brief
     *  Time in seconds to keep a message alive.
     * @remark
     *  Default value is @a 200.
     */
    StandardVariable<uint16_t> hud_messageDuration;

    /**
     * @brief
     *  Show/hide status bar.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> hud_displayStatusBars;

    /**
     * @brief
     *  Show/hide game time.
     * @remark
     *  Default value is @a false.
     */
    StandardVariable<bool> hud_displayGameTime;

    /**
     * @brief
     *  Feedback given to the player.
     * @remark
     *  Default value is @a Ego::FeedbackType::Text.
     */
    EnumVariable<Ego::FeedbackType> hud_feedback;

    /**
     * @brief
     *  Enable/disable messages.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> hud_messages_enable;

    /**
     * @brief
     *  Show/hide frames per second.
     * @remark
     *  Default value is @a false.
     */
    StandardVariable<bool> hud_displayFramesPerSecond;

    // Debug configuration section.

    /**
     * @brief
     *  Hide mouse?
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> debug_hideMouse;

	/**
	 * @brief
	 *  Render mesh's height map?
	 * @remark
	 *  Default value is @a false.
	 */
	StandardVariable<bool> debug_mesh_renderHeightMap;

	/**
	 * @brief
	 *  Render mesh's normals?
	 * @remark
	 *  Default value is @a false.
	 */
	StandardVariable<bool> debug_mesh_renderNormals;
    
    /**
     * @brief
     *  Grab mouse?
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> debug_grabMouse;
    
    /**
     * @brief
     *  Enable/disable developer mode.
     * @remark
     *  Default value is @a false.
     */
    StandardVariable<bool> debug_developerMode_enable;
    
    /**
     * @brief
     *  Enable/disable SDL image.
     * @remark
     *  Default value is @a true.
     */
    StandardVariable<bool> debug_sdlImage_enable;

public:

    /**
     * @brief
     *  Construct this Egoboo configuration with default settings.
     */
    egoboo_config_t();

    /**
     * @brief
     *  Destruct this Egoboo configuration.
     */
    virtual ~egoboo_config_t();

    /**
     * @brief
     *  Assign this Egoboo configuration the values of another Egoboo configuration.
     * @param other
     *  the other Egoboo configuration
     * @return
     *  this Egoboo configuration
     */
    egoboo_config_t& operator=(const egoboo_config_t& other);

public:

    /**
     * @brief
     *  Get the Egoboo configuration singleton.
     * @return
     *  the singleton
     */
    static egoboo_config_t& get();

};


/**
 * @brief
 *  Load the local <tt>"setup.txt"</tt>.
 * @remark
 *  This will initialize a represention fromload the configuration from the local <tt>"setup.txt"</tt>.
 *  If loading fails, the outcome of this function is equivalent to the case in which that file is empty.
 */
bool setup_begin();

/**
 * @brief
 *  Save the local <tt>"setup.txt"</tt>.
 * @remark
 *  This will write the configuration to the local <tt>"setup.txt"</tt>.
 */
bool setup_end();

/**
 * @brief
 *  Download the local <tt>"setup.txt"</tt> into an Egoob configuration.
 * @return
 *  @a true on success, @a false on failure
 * @remark
 *  If the local <tt>"setup.txt"</tt> is missing values,
 *  those value will be replaced by values from the default Egoboo configuration.
 */
bool setup_download(egoboo_config_t *cfg);

/**
 * @brief
 *  Upload an Egoboo configuration into the local <tt>"setup.txt"</tt>.
 */
bool setup_upload(egoboo_config_t *cfg);

/// Set in the VFS the basic mount points/search paths.
void setup_init_base_vfs_paths();

/// Remove from the VFS the basic mounts points/search paths.
void setup_clear_base_vfs_paths();

/// Set in the VFS the module specific mount points/search path.
bool setup_init_module_vfs_paths(const char *mod_path);

/// Remove from the VFS the module specific mount points/search paths.
void setup_clear_module_vfs_paths();

