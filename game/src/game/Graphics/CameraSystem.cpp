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
#include "game/Graphics/CameraSystem.hpp"

#include "egolib/_math.h"

#include "game/network.h"
#include "game/mesh.h"
#include "game/graphic.h"
#include "game/game.h"
#include "game/player.h"
#include "game/char.h"

#include "game/Entities/_Include.hpp"

//Define static variables
std::weak_ptr<CameraSystem> CameraSystem::_singleton;
CameraOptions CameraSystem::_cameraOptions;

CameraSystem::CameraSystem(const size_t numberOfCameras) :
	_initialized(false),
	_cameraList(),
    _mainCamera(nullptr)
{
    //Create cameras
    for(size_t i = 0; i < CLIP<size_t>(numberOfCameras, 1, MAX_CAMERAS); ++i) {
        _cameraList.push_back( std::make_shared<Camera>(_cameraOptions) );
    }

    //If there are no valid players then make free movement camera
    if(numberOfCameras == 0) {
        _cameraList[0]->setCameraMovementMode(CameraMovementMode::Free);
    }

    // we're initialized.
    _initialized = true;

    //Set camera 1 as main camera by default
    if(!_cameraList.empty()) {
        _mainCamera = _cameraList[0];
    }

    // set camera size depending on resolution
    autoFormatTargets();

    // spread the targets out over all the cameras
    autoSetTargets();

    // make sure the cameras are centered on something or there will be a graphics error
    resetAllTargets(_currentModule->getMeshPointer());
}

CameraSystem::~CameraSystem()
{
    _cameraList.clear();
    _initialized = false;
    _mainCamera = nullptr;
}

bool CameraSystem::isInitialized()
{
	return _initialized;
}

void CameraSystem::resetAll(const ego_mesh_t * mesh)
{
	if(!isInitialized()) {
		return;
	}

    // reset each camera
    for(const std::shared_ptr<Camera> &camera : _cameraList)
    {
    	camera->reset(mesh);
    }
}

void CameraSystem::updateAll( const ego_mesh_t * mesh )
{
	if(!isInitialized()) {
		return;
	}

    // update each camera
    for(const std::shared_ptr<Camera> &camera : _cameraList)
    {
    	camera->update(mesh);
    }
}

void CameraSystem::resetAllTargets( const ego_mesh_t * mesh )
{
	if(!isInitialized()) {
		return;
	}

    // update each camera
    for(const std::shared_ptr<Camera> &camera : _cameraList)
    {
    	camera->resetTarget(mesh);
    }
}

egolib_rv CameraSystem::renderAll(std::function<void(std::shared_ptr<Camera>, std::shared_ptr<Ego::Graphics::TileList>, std::shared_ptr<Ego::Graphics::EntityList>)> renderFunction)
{
    if ( NULL == renderFunction ) {
        return rv_error;
    }

    if ( !isInitialized() ) {
        return rv_fail;
    }

    //Store main camera to restore
    std::shared_ptr<Camera> storeMainCam = _mainCamera;

    for(const std::shared_ptr<Camera> &camera : _cameraList) 
    {
        // set the "global" camera pointer to this camera
        _mainCamera = camera;

	    // has this camera already rendered this frame?
        if ( camera->getLastFrame() >= 0 && static_cast<uint32_t>(camera->getLastFrame()) >= game_frame_all ) {
            continue;
        }

        // set up everything for this camera
        beginCameraMode(camera);

        // render the world for this camera
        renderFunction(camera, camera->getTileList(), camera->getEntityList());

        // undo the camera setup
        endCameraMode();

        //Set last update frame
        camera->setLastFrame(game_frame_all);
    }

    // reset the "global" camera pointer to whatever it was
    _mainCamera = storeMainCam;

    return rv_success;
}

size_t CameraSystem::getCameraIndexByID(const CHR_REF target) const
{
    if (target == INVALID_CHR_REF)  {
        return 0;
    }

    for(size_t i = 0; i < _cameraList.size(); ++i)
    {
        for(CHR_REF id : _cameraList[i]->getTrackList())
        {
            if(id == target) {
                return i;
            }
        }
    }

    return 0;
}

std::shared_ptr<Camera> CameraSystem::getCameraByChrID(const CHR_REF target) const
{
    if (target == INVALID_CHR_REF)  {
    	return _mainCamera;
    }

    for(const std::shared_ptr<Camera> &camera : _cameraList) 
    {
    	for(CHR_REF id : camera->getTrackList())
    	{
    		if(id == target) {
    			return camera;
    		}
    	}
    }

    return _mainCamera;
}

void CameraSystem::endCameraMode()
{
    // make the viewport the entire screen
    Ego::Renderer::get().setViewportRectangle(0, 0, sdl_scr.x, sdl_scr.y);

    // turn off the scissor mode
    Ego::Renderer::get().setScissorTestEnabled(false);
}


void CameraSystem::beginCameraMode( const std::shared_ptr<Camera> &camera)
{
    auto& renderer = Ego::Renderer::get();
    // scissor the output to the this area
    renderer.setScissorTestEnabled(true);
    renderer.setScissorRectangle(camera->getScreen().xmin, sdl_scr.y - camera->getScreen().ymax, camera->getScreen().xmax - camera->getScreen().xmin, camera->getScreen().ymax - camera->getScreen().ymin);

    // set the viewport
    renderer.setViewportRectangle(camera->getScreen().xmin, sdl_scr.y - camera->getScreen().ymax, camera->getScreen().xmax - camera->getScreen().xmin, camera->getScreen().ymax - camera->getScreen().ymin);
}

void CameraSystem::autoFormatTargets()
{
    if(_cameraList.empty())
    {
        return;
    }

    // 1/2 of border between panes in pixels
    static const int border = 1;

    float aspect_ratio = static_cast<float>(sdl_scr.x) / static_cast<float>(sdl_scr.y);
    bool widescreen = ( aspect_ratio > ( 4.0f / 3.0f ) );

    if ( widescreen )
    {
        switch ( _cameraList.size() )
        {
            default:
            case 1:
                // fullscreen
                _cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x, sdl_scr.y);
                break;

            case 2:
                // wider than tall, so windows are side-by side
                _cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y);
                _cameraList[1]->setScreen(sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y);
                break;

            case 3:
                // wider than tall, so windows are side-by side
                _cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x / 3.0f - border, sdl_scr.y);
                _cameraList[1]->setScreen(sdl_scr.x / 3.0f + border, 0.0f, 2.0f * sdl_scr.x / 3.0f - border, sdl_scr.y);
                _cameraList[2]->setScreen(2.0f * sdl_scr.x / 3.0f + border, 0.0f, sdl_scr.x, sdl_scr.y);
                break;

            case 4:
                // 4 panes
                _cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y * 0.5f - border);
                _cameraList[1]->setScreen(sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border);
                _cameraList[2]->setScreen(0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x * 0.5f - border, sdl_scr.y);
                _cameraList[3]->setScreen(sdl_scr.x * 0.5f + border, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y);
                break;
        }
    }
    else
    {
        switch ( _cameraList.size() )
        {
            default:
            case 1:
                // fullscreen
                _cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x, sdl_scr.y);
                break;

            case 2:
                if ( sdl_scr.x  >= sdl_scr.y )
                {
                    // wider than tall, so windows are side-by side
                    _cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y);
                    _cameraList[1]->setScreen(sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y);
                }
                else
                {
                    // taller than wide so, windows are one-over-the-other
                    _cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border);
                    _cameraList[1]->setScreen(0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y);
                }
                break;

            case 3:
                // more square, so 4 panes, but one is blank
            	_cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y * 0.5f - border);
            	_cameraList[1]->setScreen(sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border);
            	_cameraList[2]->setScreen(0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y);
            	//_cameraList[3] does not exist, so blank
                break;

            case 4:
                // 4 panes
            	_cameraList[0]->setScreen(0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y * 0.5f - border);
            	_cameraList[1]->setScreen(sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border);
            	_cameraList[2]->setScreen(0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x * 0.5f - border, sdl_scr.y);
            	_cameraList[3]->setScreen(sdl_scr.x * 0.5f + border, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y);
                break;
        }
    }
}

void CameraSystem::autoSetTargets()
{
	if(_cameraList.empty()) {
		return;
	}

    // find a valid camera
    size_t cameraIndex = 0;
   
    // put all the valid players into camera 0
    for ( size_t cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        // only look at valid players
        player_t * ppla = PlaStack.get_ptr( cnt );
        if ( !ppla->valid || INVALID_CHR_REF == ppla->index ) continue;

        // only look at local players
        if ( NULL == ppla->pdevice ) continue;

        // wrap around if there are less cameras than players
        if(cameraIndex >= _cameraList.size()) {
            cameraIndex = 0;
        }

        // store the target
        _cameraList[cameraIndex]->addTrackTarget(ppla->index);
        cameraIndex++;
    }
}

CameraOptions& CameraSystem::getCameraOptions()
{
    return _cameraOptions;
}

std::shared_ptr<CameraSystem> CameraSystem::request(size_t numberOfCameras) 
{
    //Check if it is already allocated
    std::shared_ptr<CameraSystem> allocatedSystem = _singleton.lock();
    if(allocatedSystem && allocatedSystem->_cameraList.size() >= numberOfCameras) {
        return _singleton.lock();
    }

    //Allocate new one
    allocatedSystem = std::make_shared<CameraSystem>(numberOfCameras);
    _singleton = allocatedSystem;
    return allocatedSystem;
}