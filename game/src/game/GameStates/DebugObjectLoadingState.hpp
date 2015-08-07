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

/// @file game/GameStates/DebugObjectLoadingState.cpp
/// @details Debugging state where one can debug loading objects
///          from the global repository or individual modules.
/// @author Johan Jansen, penguinflyer5234

#pragma once

#include "game/GameStates/GameState.hpp"

//Forward declarations
class ModuleProfile;
class Label;
class ScrollableList;

class DebugObjectLoadingState : public GameState
{
public:
    DebugObjectLoadingState();
    ~DebugObjectLoadingState();
    
    void update() override;
    
    void beginState() override;
    
protected:
    void drawContainer() override;
    
    void loadObjectData();
    
    /**
     * ZF> This function is a place-holder hack until we get proper threaded loading working
     **/
    void singleThreadRedrawHack(const std::string &loadingText);
    
private:
    std::atomic_bool _finishedLoading;
    std::thread _loadingThread;
    std::shared_ptr<ScrollableList> _scrollableList;
    
    struct ObjectGUIContainer;
    struct ModuleLoader;
    struct GlobalLoader;
    struct GrowableLabel;
    std::vector<std::shared_ptr<ModuleLoader>> _moduleList;
    std::deque<std::shared_ptr<ObjectGUIContainer>> _toLoad;
    std::shared_ptr<ModuleLoader> _currentLoader;
    
    void addToQueue(const std::shared_ptr<ModuleLoader> &toAdd);
    void addToQueue(const std::shared_ptr<ObjectGUIContainer> &toAdd);
};