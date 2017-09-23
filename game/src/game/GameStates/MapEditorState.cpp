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

/// @file game/GameStates/MapEditorState.cpp
/// @details Main state where the players are currently playing a module
/// @author Johan Jansen

#include "game/GameStates/MapEditorState.hpp"
#include "game/GameStates/InGameMenuState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/MiniMap.hpp"
#include "game/GUI/Button.hpp"
#include "game/game.h"
#include "game/Graphics/TextureAtlasManager.hpp"
#include "game/Graphics/TileList.hpp"
#include "game/Graphics/CameraSystem.hpp"

#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"

namespace Ego
{
namespace GameStates
{

MapEditorState::MapEditorState(std::shared_ptr<ModuleProfile> module) :
    _miniMap(std::make_shared<Ego::GUI::MiniMap>()),
    _modeButtons(),
    _editMode(EditorMode::MAP_EDIT_NONE)
{
    //Add minimap to the list of GUI components to render
    _miniMap->setSize(Vector2f(Ego::GUI::MiniMap::MAPSIZE, Ego::GUI::MiniMap::MAPSIZE));
    _miniMap->setPosition(Point2f(0, _gameEngine->getUIManager()->getScreenHeight()-_miniMap->getHeight()));
    _miniMap->setVisible(true);
    addComponent(_miniMap);

    //Load the module
    loadModuleData(module);

    //Add edit modes
    addModeEditButton(EditorMode::MAP_EDIT_OBJECTS, "Objects");
    addModeEditButton(EditorMode::MAP_EDIT_PASSAGES, "Passages");
    addModeEditButton(EditorMode::MAP_EDIT_MESH, "Mesh");

    //Center the camera in the middle of the map
    Vector3f mapCenter;
    mapCenter.x() = _currentModule->getMeshPointer()->_info.getTileCountX()*Info<float>::Grid::Size() * 0.5f;
    mapCenter.y() = _currentModule->getMeshPointer()->_info.getTileCountY()*Info<float>::Grid::Size() * 0.5f;
    mapCenter.z() = _currentModule->getMeshPointer()->getElevation(Vector2f(mapCenter.x(), mapCenter.y()), false);
    CameraSystem::get().getMainCamera()->setPosition(mapCenter);
}

void MapEditorState::addModeEditButton(EditorMode mode, const std::string &label)
{
    auto editModeButton = std::make_shared<Ego::GUI::Button>(label);
    editModeButton->setSize(Vector2f(120, 30));
    editModeButton->setPosition(Point2f(_modeButtons.size() * (editModeButton->getWidth() + 5), 0));
    editModeButton->setOnClickFunction([this, mode, editModeButton]{
        _editMode = mode;
        for(const auto& button : _modeButtons) {
            button->setEnabled(true);
        }
        editModeButton->setEnabled(false);
    });
    addComponent(editModeButton);
    _modeButtons.push_back(editModeButton);    
}

void MapEditorState::update()
{
    // Get immediate mode state for the rest of the game
    Ego::Input::InputSystem::get().update();

    //Rebuild the quadtree for fast object lookup
    _currentModule->getObjectHandler().updateQuadTree(0.0f, 0.0f, _currentModule->getMeshPointer()->_info.getTileCountX()*Info<float>::Grid::Size(),
		                                                          _currentModule->getMeshPointer()->_info.getTileCountY()*Info<float>::Grid::Size());

    //Always reveal all invisible monsters and objects in Map Editor mode
    local_stats.seeinvis_level = 100;

    //Animate water
    _currentModule->getWater().update();

    //Update camera movement
    CameraSystem::get().getMainCamera()->updateFreeControl();
}

void MapEditorState::drawContainer(Ego::GUI::DrawingContext& drawingContext)
{
    CameraSystem::get().renderAll(gfx_system_render_world);
    draw_hud();

    //Draw passages?
    if(_editMode == EditorMode::MAP_EDIT_PASSAGES) {
        draw_passages(*CameraSystem::get().getMainCamera());
    }
}

void MapEditorState::beginState()
{
    // in-game settings
    Ego::GraphicsSystemNew::get().setCursorVisibility(true);
    Ego::GraphicsSystem::get().window->setGrabEnabled(egoboo_config_t::get().debug_grabMouse.getValue());
}

bool MapEditorState::notifyKeyboardKeyPressed(const Ego::Events::KeyboardKeyPressedEventArgs& e)
{
    switch(e.key())
    {
        case SDLK_ESCAPE:
            _gameEngine->pushGameState(std::make_shared<InGameMenuState>(*this));
        return true;
    }

    return Container::notifyKeyboardKeyPressed(e);
}

void MapEditorState::loadModuleData(std::shared_ptr<ModuleProfile> module)
{
    //Make sure any old data is cleared first
    game_quit_module();

    // Reset all loaded "profiles" in the "profile system".
    ProfileSystem::get().reset();

    // do some graphics initialization
    gfx_system_make_enviro();

    // try to start a new module
    game_begin_module(module);

    // set up the cameras *after* game_begin_module() or the player devices will not be initialized
    // and camera_system_begin() will not set up the correct view
    CameraSystem::get().initialize(1);
    CameraSystem::get().getMainCamera()->setCameraMovementMode(CameraMovementMode::Free);

    // make sure the per-module configuration settings are correct
    config_synch(egoboo_config_t::get(), true, false);

    //Have to do this function in the OpenGL context thread or else it will fail
    Ego::Graphics::TextureAtlasManager::get().loadTileSet();
}


} //GameStates
} //Ego