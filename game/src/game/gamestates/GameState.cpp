#include "game/gamestates/GameState.hpp"

GameState::GameState() :
	_terminateStateRequested(false)
{
	//ctor
}

void GameState::endState()
{
	_terminateStateRequested = true;
}

bool GameState::isEnded() const
{
	return _terminateStateRequested;
}

void GameState::beginState()
{
	//default does nothing
}
