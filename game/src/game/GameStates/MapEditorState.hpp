#pragma once

#include "game/GameStates/GameState.hpp"

// Forward declarations.
class CameraSystem;
namespace Ego {
namespace GUI {
class MiniMap;
class Button;
} // namespace GUI
} // namespace Ego
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

    bool notifyKeyboardKeyPressed(const Ego::Events::KeyboardKeyPressedEvent& e) override;

    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        drawContainer(drawingContext);
    }
protected:
    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override;

private:
	enum class EditorMode
	{
		MAP_EDIT_NONE,			//Only move around and look, no modify
		MAP_EDIT_OBJECTS,		//Place, move, inspect and modify objects
		MAP_EDIT_PASSAGES,		//Resize, move rename and inspect passages
		MAP_EDIT_MESH,			//Make floor tiles or wall tiles
		MAP_EDIT_MESH_FLAGS,	//Flag mesh tiles as wall, water, ice, etc.
		MAP_EDIT_MESH_TEXTURE	//Paintbrush for mesh
	};

	void loadModuleData(std::shared_ptr<ModuleProfile> module);

	void addModeEditButton(EditorMode mode, const std::string &label);

private:
    std::shared_ptr<Ego::GUI::MiniMap> _miniMap;
    std::vector<std::shared_ptr<Ego::GUI::Button>> _modeButtons;
    EditorMode _editMode;
};

} //GameStates
} //Ego