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

/// @file game/GameStates/LoadingState.hpp
/// @details Main state where the players are currently playing a module
/// @todo Wrong doc comment.
/// @author Johan Jansen

#pragma once

#include "game/GameStates/GameState.hpp"

// Forward declarations.
class ModuleProfile;

namespace Ego {
namespace GUI {
class Label;
class ProgressBar;
} // namespace GUI
} // namespace Ego

class LoadingState : public GameState
{

public:

    LoadingState(std::shared_ptr<ModuleProfile> module, const std::list<std::string> &playersToLoad);

    ~LoadingState();

    void update() override;

    void beginState() override;
    
    void draw() override {
        drawContainer();
    }
protected:

    void drawContainer() override;

    void loadModuleData();

    /**
     * @brief
     *    Actually loads all data for the Characters that players have picked.
     */
    bool loadPlayers();

    const std::string getRandomHint() const;

    /// @author ZF
    /// @details This function loads all module specific hints and tips. If this fails, the game will
    ///       default to the global hints and tips instead
      bool loadLocalModuleHints();

    /// @author ZF
    /// @details This function loads all of the game hints and tips
    bool loadGlobalHints();

    void setProgressText(const std::string &loadingText, const uint8_t progress);

private:
    std::thread _loadingThread;
    std::shared_ptr<Ego::GUI::Label> _loadingLabel;
    const std::shared_ptr<ModuleProfile> _loadModule;
    std::list<std::string> _playersToLoad;
    std::shared_ptr<Ego::GUI::ProgressBar> _progressBar;

    std::vector<std::string> _globalGameTips;        //Generic game tips for the whole game
    std::vector<std::string> _localGameTips;        //Game tips specific to this module
};
