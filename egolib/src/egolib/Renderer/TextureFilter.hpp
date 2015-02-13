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

/// @file   egolib/Renderer/TextureFilter.h
/// @brief  An enumeration of texture filtering methods.
/// @author Michael Heilmann

#pragma once

namespace Ego
{
	/**
	 * @brief
	 *	An enumeration of texture filtering methods.
	 */
	enum TextureFilter
	{

		UNFILTERED,

		LINEAR,

		MIPMAP,

		BILINEAR,

		TRILINEAR_1,

		TRILINEAR_2,

		ANISOTROPIC,

		FILTER_COUNT,

	};
};