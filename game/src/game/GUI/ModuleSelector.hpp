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

/// @file game/GUI/ModuleSelector.cpp
/// @details GUI widget to select which module to play
/// @author Johan Jansen
#pragma once

#include "game/GUI/Button.hpp"
#include "game/GUI/ComponentContainer.hpp"

//Forward declarations
class ModuleProfile;
class Button;

class ModuleSelector : public ComponentContainer, public GUIComponent
{
    public:
        ModuleSelector(const std::vector<std::shared_ptr<ModuleProfile>> &modules);

        void draw() override { drawAll(); }

        //Disable copying class
        ModuleSelector(const ModuleSelector& copy) = delete;
        ModuleSelector& operator=(const ModuleSelector&) = delete;

        void notifyModuleListUpdated();

        /**
        * Ensure that this class inherits defaults for these methods from ComponentContainer and not GUIComponent
        **/
	    bool notifyMouseMoved(const int x, const int y) override 					 {return ComponentContainer::notifyMouseMoved(x, y);}
	    bool notifyKeyDown(const int keyCode) override 								 {return ComponentContainer::notifyKeyDown(keyCode);}
	    bool notifyMouseClicked(const int button, const int x, const int y) override {return ComponentContainer::notifyMouseClicked(button, x, y);}
	    bool notifyMouseScrolled(const int amount) override;

	    const std::shared_ptr<ModuleProfile>& getSelectedModule() const;

    protected:
	    void drawContainer() override;

	    //Local class
	    class ModuleButton : public Button
	    {
	    public:
	    	ModuleButton(ModuleSelector* selector, const uint8_t offset);

	    	void draw() override;

	    	//Disable copying class
        	ModuleButton(const ModuleButton& copy) = delete;
        	ModuleButton& operator=(const ModuleButton&) = delete;

        private:
	        ModuleSelector *_moduleSelector;
	        uint8_t _offset;
	    };

    private:
        size_t _startIndex;
        const std::vector<std::shared_ptr<ModuleProfile>> &_modules;
        std::shared_ptr<Button> _nextModuleButton;
        std::shared_ptr<Button> _previousModuleButton;
        std::shared_ptr<ModuleProfile> _selectedModule;
};
