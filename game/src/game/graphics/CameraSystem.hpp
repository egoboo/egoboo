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
#pragma once

#include "game/egoboo_typedef.h"
#include "game/input.h"
#include "game/graphics/ExtendedCamera.hpp"

// Forward declaration.
struct ego_mesh_t;

constexpr size_t MAX_CAMERAS = MAX_LOCAL_PLAYERS;

class CameraSystem
{
public:
	
	CameraSystem();

	/**
	* @return true if the camera system has been initialized and can be used
	**/
	bool isInitialized();

	/**
	* @brief initializes the camera system, must be called before using
	**/
	void begin(const size_t numberOfCameras);

	/**
	* @brief frees any resources locked by the camera ssytem
	**/
	void end();

	void resetAll( const ego_mesh_t * pmesh );
	void updateAll( const ego_mesh_t * pmesh );
	void resetAllTargets( const ego_mesh_t * pmesh );

	egolib_rv renderAll(std::function<void(std::shared_ptr<ExtendedCamera>, int, int)> prend);

	std::shared_ptr<ExtendedCamera> getCameraByChrID(const CHR_REF target) const;

	size_t getCameraIndexByID(const CHR_REF target) const;

	inline const std::vector<std::shared_ptr<ExtendedCamera>>& getCameraList() const {return _cameraList;}

    /**
    * @brief write access to global camera options
    **/
    CameraOptions& getCameraOptions();

	inline std::shared_ptr<ExtendedCamera> getMainCamera() const {return _mainCamera;}

private:

	GLint beginExtendedCameraMode(const std::shared_ptr<ExtendedCamera> &camera);
	void endExtendedCameraMode( GLint mode );

	/**
	* @brief Determines the size of each camera depending on number of cameras and screen resolution.
	*		 This also handles split cameras.
	**/
    void autoFormatTargets();

    /**
    * @brief spread the targets out over all the cameras
    **/
    void autoSetTargets();

private:
	bool _initialized;
	CameraOptions _cameraOptions;
	std::vector<std::shared_ptr<ExtendedCamera>> _cameraList;
	std::shared_ptr<ExtendedCamera> _mainCamera;
};
