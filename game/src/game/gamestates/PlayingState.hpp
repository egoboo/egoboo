#pragma once

#include "game/gamestates/GameState.hpp"

class PlayingState : public GameState
{
public:
	PlayingState();

	void update() override;

	void beginState() override;
	
protected:
	void drawContainer() override;
};
