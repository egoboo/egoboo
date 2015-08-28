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

/// @file   egolib/Renderer/BlendFunction.hpp
/// @brief  Enumeration of blend functions used for depth/stencil buffer operations and others.
/// @author Michael Heilmann

namespace Ego
{

/**
 * @brief
 *  Blend functions used for the depth/stencil buffer operations and others.
 * @author
 *  Michael Heilmann
 * @see
 *	Ego::Renderer::setBlendFunction
 */
enum class BlendFunction
{
	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	Zero,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	One,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	SourceColour,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	OneMinusSourceColour,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	DestinationColour,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	OneMinusDestinationColour,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	SourceAlpha,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	OneMinusSourceAlpha,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	DestinationAlpha,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	OneMinusDestinationAlpha,

	/**
	* @see
	*  Ego::Renderer::setBlendFunction
	*/
	ConstantColour,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	OneMinusConstantColour,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	ConstantAlpha,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	OneMinusConstantAlpha,

	/**
	 * @see
	 *  Ego::Renderer::setBlendFunction
	 */
	SourceAlphaSaturate,

};

} // namespace Ego
