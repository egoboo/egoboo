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

#include "egolib/Graphics/Viewport.hpp"
#include "game/mesh.h"
#include "game/graphic.h"
#include "game/game.h"
#include "game/Logic/Player.hpp"
#include "game/Core/GameEngine.hpp"

#include "egolib/Entities/_Include.hpp"

CameraSystem::CameraSystem() :
	_initialized(false),
	_cameraList(),
    _mainCamera(nullptr),
    _cameraOptions()
{
    //ctor
}

CameraSystem::~CameraSystem()
{
    _cameraList.clear();
    _initialized = false;
    _mainCamera = nullptr;
}

void CameraSystem::initialize(const size_t numberOfCameras)
{
    // we're initialized.
    _initialized = true;

    //Create cameras
    _cameraList.clear();
    for(size_t i = 0; i < Ego::Math::constrain<size_t>(numberOfCameras, 1, MAX_CAMERAS); ++i) {
        _cameraList.push_back(std::make_shared<Camera>(_cameraOptions));
    }

    //Set camera 1 as main camera by default
    _mainCamera = _cameraList[0];

    //If there are no valid players then make free movement camera
    if(numberOfCameras == 0) {
        _cameraList[0]->setCameraMovementMode(CameraMovementMode::Free);
    }

    // set camera size depending on resolution
    autoFormatTargets();

    // spread the targets out over all the cameras
    autoSetTargets();

    // make sure the cameras are centered on something or there will be a graphics error
    resetAllTargets(_currentModule->getMeshPointer().get());
}

bool CameraSystem::isInitialized()
{
	return _initialized;
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
        if ( camera->getLastFrame() >= 0 && static_cast<uint32_t>(camera->getLastFrame()) >= _gameEngine->getNumberOfFramesRendered()) {
            continue;
        }

        // set up everything for this camera
        beginCameraMode(camera);

        // render the world for this camera
        renderFunction(camera, camera->getTileList(), camera->getEntityList());

        // undo the camera setup
        endCameraMode();

        //Set last update frame
        camera->setLastFrame(_gameEngine->getNumberOfFramesRendered());
    }

    // reset the "global" camera pointer to whatever it was
    _mainCamera = storeMainCam;

    return rv_success;
}

size_t CameraSystem::getCameraIndex(ObjectRef targetRef) const {
    if (ObjectRef::Invalid == targetRef) {
        return 0;
    }

    for(size_t i = 0; i < _cameraList.size(); ++i) {
        for (ObjectRef objectRef : _cameraList[i]->getTrackList()) {
            if(objectRef == targetRef) {
                return i;
            }
        }
    }

    return 0;
}

std::shared_ptr<Camera> CameraSystem::getCamera(ObjectRef targetRef) const
{
    if (ObjectRef::Invalid == targetRef)  {
    	return _mainCamera;
    }

    for(const std::shared_ptr<Camera> &camera : _cameraList) {
    	for(ObjectRef objectRef : camera->getTrackList()) {
    		if(objectRef == targetRef) {
    			return camera;
    		}
    	}
    }

    return _mainCamera;
}

void CameraSystem::endCameraMode()
{
    // make the viewport the entire screen
    auto drawableSize = Ego::GraphicsSystem::get().window->getDrawableSize();;
    Ego::Renderer::get().setViewportRectangle(0, 0, drawableSize.x(), drawableSize.y());

    // turn off the scissor mode
    Ego::Renderer::get().setScissorTestEnabled(false);
}


void CameraSystem::beginCameraMode( const std::shared_ptr<Camera> &camera)
{
    auto& renderer = Ego::Renderer::get();
    auto drawableSize = Ego::GraphicsSystem::get().window->getDrawableSize();
    // scissor the output to the this area
    renderer.setScissorTestEnabled(true);
    renderer.setScissorRectangle(camera->getViewport().getLeftPixels(), drawableSize.y() - (camera->getViewport().getTopPixels() + camera->getViewport().getHeightPixels()), camera->getViewport().getWidthPixels(), camera->getViewport().getHeightPixels());

    // set the viewport
    renderer.setViewportRectangle(camera->getViewport().getLeftPixels(), drawableSize.y() - (camera->getViewport().getTopPixels() + camera->getViewport().getHeightPixels()), camera->getViewport().getWidthPixels(), camera->getViewport().getHeightPixels());
}

void CameraSystem::autoFormatTargets()
{
    if(_cameraList.empty())
    {
        return;
    }

    // 1/2 of border between panes in pixels
    static const int border = 1;
    auto windowSize = Ego::GraphicsSystem::get().window->getSize();
    float aspect_ratio = windowSize.x() / windowSize.y();
    bool widescreen = ( aspect_ratio > ( 4.0f / 3.0f ) );

    if ( widescreen )
    {
        switch ( _cameraList.size() )
        {
            default:
            case 1:
                // fullscreen
                _cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x(), windowSize.y());
                break;

            case 2:
                // wider than tall, so windows are side-by side
                _cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x() * 0.5f - border, windowSize.y());
                _cameraList[1]->setScreen(windowSize.x() * 0.5f + border, 0.0f, windowSize.x(), windowSize.y());
                break;

            case 3:
                // wider than tall, so windows are side-by side
                _cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x() / 3.0f - border, windowSize.y());
                _cameraList[1]->setScreen(windowSize.x() / 3.0f + border, 0.0f, 2.0f * windowSize.x() / 3.0f - border, windowSize.y());
                _cameraList[2]->setScreen(2.0f * windowSize.x() / 3.0f + border, 0.0f, windowSize.x(), windowSize.y());
                break;

            case 4:
                // 4 panes
                _cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x() * 0.5f - border, windowSize.y() * 0.5f - border);
                _cameraList[1]->setScreen(windowSize.x() * 0.5f + border, 0.0f, windowSize.x(), windowSize.y() * 0.5f - border);
                _cameraList[2]->setScreen(0.0f, windowSize.y() * 0.5f + border, windowSize.x() * 0.5f - border, windowSize.y());
                _cameraList[3]->setScreen(windowSize.x() * 0.5f + border, windowSize.y() * 0.5f + border, windowSize.x(), windowSize.y());
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
                _cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x(), windowSize.y());
                break;

            case 2:
                if (windowSize.x() >= windowSize.y())
                {
                    // wider than tall, so windows are side-by side
                    _cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x() * 0.5f - border, windowSize.y());
                    _cameraList[1]->setScreen(windowSize.x() * 0.5f + border, 0.0f, windowSize.x(), windowSize.y());
                }
                else
                {
                    // taller than wide so, windows are one-over-the-other
                    _cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x(), windowSize.y() * 0.5f - border);
                    _cameraList[1]->setScreen(0.0f, windowSize.y() * 0.5f + border, windowSize.x(), windowSize.y());
                }
                break;

            case 3:
                // more square, so 4 panes, but one is blank
            	_cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x() * 0.5f - border, windowSize.y() * 0.5f - border);
            	_cameraList[1]->setScreen(windowSize.x() * 0.5f + border, 0.0f, windowSize.x(), windowSize.y() * 0.5f - border);
            	_cameraList[2]->setScreen(0.0f, windowSize.y() * 0.5f + border, windowSize.x(), windowSize.y());
            	//_cameraList[3] does not exist, so blank
                break;

            case 4:
                // 4 panes
            	_cameraList[0]->setScreen(0.0f, 0.0f, windowSize.x() * 0.5f - border, windowSize.y() * 0.5f - border);
            	_cameraList[1]->setScreen(windowSize.x() * 0.5f + border, 0.0f, windowSize.x(), windowSize.y() * 0.5f - border);
            	_cameraList[2]->setScreen(0.0f, windowSize.y() * 0.5f + border, windowSize.x() * 0.5f - border, windowSize.y());
            	_cameraList[3]->setScreen(windowSize.x() * 0.5f + border, windowSize.y() * 0.5f + border, windowSize.x(), windowSize.y());
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
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList()) {

        // wrap around if there are less cameras than players
        if(cameraIndex >= _cameraList.size()) {
            cameraIndex = 0;
        }

        // store the target
        _cameraList[cameraIndex]->addTrackTarget(player->getObject()->getObjRef());
        cameraIndex++;
    }
}

CameraOptions& CameraSystem::getCameraOptions()
{
    return _cameraOptions;
}
