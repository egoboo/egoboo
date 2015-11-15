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
/// @file game/Graphics/CameraSystem.hpp
/// @author Johan Jansen
#pragma once

#include "game/egoboo_typedef.h"
#include "game/input.h"
#include "game/Graphics/Camera.hpp"

// Forward declaration.
class ego_mesh_t;

static constexpr size_t MAX_CAMERAS = MAX_LOCAL_PLAYERS;

/**
 * @brief
 *  A multi-camera system consisting of 1 to @a n cameras with indices @a 0 to <tt>n-1</tt>.
 *  The camera at index @a 0 is called the "main camera".
 */
class CameraSystem
{
public:	
	CameraSystem(const size_t numberOfCameras);

	~CameraSystem();

	/**
	 * @return true if the camera system has been initialized and can be used
	 */
	bool isInitialized();

	void resetAll( const ego_mesh_t * mesh );
	void updateAll( const ego_mesh_t * mesh );
	void resetAllTargets( const ego_mesh_t * mesh );

	egolib_rv renderAll(std::function<void(std::shared_ptr<Camera>, std::shared_ptr<Ego::Graphics::TileList>, std::shared_ptr<Ego::Graphics::EntityList>)> renderFunction);

	/**
	 * @brief
	 *  Get the first camera tracking a target.
	 * @param targetRef
	 *  the target reference
	 * @return
	 *  the first camera tracking the target.
	 *  If no camera is tracking the target or the target reference is invalid, the main camera is returned.
	 */
	std::shared_ptr<Camera> getCamera(ObjectRef targetRef) const;

	/**
	 * @brief
	 *  Get the first camera tracking a target.
	 * @param targetRef
	 *  the target reference
	 * @return
	 *  the first camera tracking the target.
	 *  If no camera is tracking the target or the target reference is invalid, the index of the main camera is returned.
	 */
	size_t getCameraIndex(ObjectRef targetRef) const;

	inline const std::vector<std::shared_ptr<Camera>>& getCameraList() const {return _cameraList;}

    /**
     * @brief write access to global camera options
     */
    static CameraOptions& getCameraOptions();

	inline std::shared_ptr<Camera> getMainCamera() const {return _mainCamera;}

	/**
	* @brief
	*	Singleton accessor for CameraSystem
	**/
	static std::shared_ptr<CameraSystem> get() {return _singleton.lock();}

	/**
	* @brief
	*	Request singleton access or create it if needed
	**/
	static std::shared_ptr<CameraSystem> request(size_t numberOfCameras);

private:

	void beginCameraMode(const std::shared_ptr<Camera> &camera);
	void endCameraMode();

	/**
	 * @brief Determines the size of each camera depending on number of cameras and screen resolution.
	 *		 This also handles split cameras.
	 */
    void autoFormatTargets();

    /**
     * @brief spread the targets out over all the cameras
     */
    void autoSetTargets();

private:
	bool _initialized;
	std::vector<std::shared_ptr<Camera>> _cameraList;
	std::shared_ptr<Camera> _mainCamera;

	static std::weak_ptr<CameraSystem> _singleton;
	static CameraOptions _cameraOptions;
};
