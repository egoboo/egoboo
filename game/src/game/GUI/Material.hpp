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

/// @file game/GUI/Material.hpp
/// @brief Material definitions for GUI rendering.
/// @todo Should be based on Materials in general.
/// @author Michael Heilmann

#pragma once

#include "game/egoboo.h"

namespace Ego {
namespace GUI {

/// @brief A material definition for the GUI.
/// @remark Materials are neither copy-constructible nor assignable.
class Material {
private:
	/// @brief A pointer to the texture if any, a null pointer otherwise.
	std::shared_ptr<const Texture> _texture;
	/// @brief The colour.
	Math::Colour4f _colour;
	/// @brief If alpha blending is enabled.
	bool _isAlphaBlendingEnabled;
	
public:
	/// @brief Construct this material.
	/// @param texture a pointer to the texture if any, a null pointer otherwise.
	/// @param colour the colour
	/// @param isAlphaBlendingEnabled if alpha blending is enabled
	Material(const std::shared_ptr<const Texture>& texture, const Colour4f& colour, bool isAlphaBlendingEnabled);
    Material() : Material(nullptr, Colour4f::white(), true) {}
	
public:
	/// @brief Apply the material to the renderer state.
	virtual void apply() const;

public:
    /// @brief Get if alpha blending is enabled.
    /// @return @a true if alpha blending is enabled, @a false otherwise
    bool isAlphaBlendingEnabled() const;
    /// @brief Set if alpha blending is enabled.
    /// @param @a true enables alpha blending, @a false disables it
    void setAlphaBlendingEnabled(bool isAlphaBlendingEnabled);

    /// @brief Get the texture.
    /// @return a pointer to the texture if any, a null pointer otherwise
    const std::shared_ptr<const Texture>& getTexture() const;
    /// @brief Set the texture.
    /// @param texture a pointer to the texture if any, a null pointer otherwise
    void setTexture(const std::shared_ptr<const Texture>& texture);

    /// @brief Get the colour.
    /// @return the colour
    const Math::Colour4f& getColour() const;
    /// @brief Set the colour.
    /// @param colour the colour
    void setColour(const Math::Colour4f& colour);
	
protected:
	Material(const Material&) = delete;
	const Material& operator=(const Material&) = delete;
};

} // namespace GUI
} // namespace Ego