#pragma once

#include "game/gui/ComponentContainer.hpp"

class GameState : public ComponentContainer
{
public:
	GameState();

	void endState();

	bool isEnded() const;

	virtual void update() = 0;

	virtual void beginState();

protected:
	virtual void drawContainer() override = 0;

private:
	bool _terminateStateRequested;
};
