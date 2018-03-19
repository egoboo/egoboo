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

/// @file egolib/game/GUI/Material.cpp
/// @brief Material definitions for GUI rendering.
/// @author Michael Heilmann

#include "egolib/game/GUI/Material.hpp"

namespace Ego::GUI {

Material::Material(const std::shared_ptr<const Texture>& texture, const Colour4f& colour, bool isAlphaBlendingEnabled)
	: _texture(texture), _colour(colour), _isAlphaBlendingEnabled(isAlphaBlendingEnabled)
	{}

void Material::apply() const {
	auto& renderer = Renderer::get();
	
	// (1) texture
	renderer.getTextureUnit().setActivated(_texture.get());
	
	// (2) colour
	renderer.setColour(_colour);

	// (3) alpha
	if (_isAlphaBlendingEnabled) {
		renderer.setBlendingEnabled(true);
		renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);

		renderer.setAlphaTestEnabled(true);
		renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);
	} else {
		renderer.setBlendingEnabled(false);
		renderer.setAlphaTestEnabled(false);
	}
}

bool Material::isAlphaBlendingEnabled() const {
    return _isAlphaBlendingEnabled;
}

void Material::setAlphaBlendingEnabled(bool isAlphaBlendingEnabled) {
    this->_isAlphaBlendingEnabled = isAlphaBlendingEnabled;
}

const std::shared_ptr<const Texture>& Material::getTexture() const {
    return _texture;
}

void Material::setTexture(const std::shared_ptr<const Texture>& texture) {
    this->_texture = texture;
}

const Colour4f& Material::getColour() const {
    return _colour;
}

void Material::setColour(const Colour4f& colour) {
    this->_colour = colour;
}

} // namespace Ego::GUI
