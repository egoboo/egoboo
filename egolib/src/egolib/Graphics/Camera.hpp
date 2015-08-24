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

/// @file     egolib/Graphics/Camera.hpp
/// @brief    Common interface of all cameras

#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Graphics {

struct Camera {

protected:

	/**
	 * @brief
	 *  Construct this camera interface.
	 */
	Camera() {}

	/**
	 * @brief
	 *  Destruct this camera interface.
	 */
	virtual ~Camera() {}

public:

	/**
	 * @brief
	 *  Get the view matrix.
	 * @return
	 *  the view matrix.
	 */
	virtual const Matrix4f4f& getViewMatrix() const = 0;

	/**
	 * @brief
	 *	Get the projection matrix of this camera.
	 * @return
	 *	the projection matrix of this camera
	 */
	virtual const Matrix4f4f& getProjectionMatrix() const = 0;

public:

	/**
	 * @brief
	 *  Get the position of the camera
	 * @return
	 *  the position of the camera.
	 */
	virtual const Vector3f& getPosition() const = 0;

public:

	/**
	 * @brief
	 *	Get the up vector of this camera.
	 * @return
	 *	the up vector of this camera
	 */
	virtual const Vector3f& getUp() const = 0;

	/**
	 * @brief
	 *	Get the right vector of this camera.
	 * @return
	 *	the right vector of this camera
	 */
	virtual const Vector3f& getRight() const = 0;

	/**
	 * @brief
	 *  Get the forward vector of this camera.
	 * @return
	 *	the forward vector of this camera
	 */
	virtual const Vector3f& getForward() const = 0;

};

} // namespace Graphics
} // namespace Ego
