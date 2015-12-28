#pragma once

#include "game/GameStates/GameState.hpp"

//Forward declaration
class CameraSystem;
class MiniMap;
class ModuleProfile;

namespace Ego
{
namespace GameStates
{

class MapEditorState : public GameState
{
public:
    MapEditorState(std::shared_ptr<ModuleProfile> module);

    void update() override;

    void beginState() override;

    bool notifyKeyDown(const int keyCode) override;


protected:
    void drawContainer() override;

private:
	void loadModuleData(std::shared_ptr<ModuleProfile> module);

private:
	std::shared_ptr<CameraSystem> _cameraSystem;
    std::shared_ptr<MiniMap> _miniMap;
};

} //GameStates
} //Ego