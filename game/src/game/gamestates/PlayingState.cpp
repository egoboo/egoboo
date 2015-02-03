#include "game/gamestates/PlayingState.hpp"
#include "egolib/egoboo_setup.h"

PlayingState::PlayingState()
{
	//ctor
}

void PlayingState::update()
{
    
}

void PlayingState::drawContainer()
{

}

void PlayingState::beginState()
{
	// in-game settings
    SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
    SDL_WM_GrabInput( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
}