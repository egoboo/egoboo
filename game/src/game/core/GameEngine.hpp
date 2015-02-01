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
/// @author Johan Jansen
#pragma once

#include "egolib/platform.h"

class GameEngine
{
public:
	GameEngine();

	void start();

	inline bool isRunning() const {return !_terminateRequested;}

	void update();

	void shutdown();

private:
	void updateOneFrame();

	void renderOneFrame();

	bool initialize();

	void uninitialize();

protected:
	static const uint32_t TARGET_FPS = 60;	///< Desired frame renders per second
	static const uint32_t TARGET_UPS = 30;	///< Desired game logic updates per second

	static const uint32_t DELAY_PER_RENDER_FRAME = 1000 / TARGET_FPS; ///< milliseconds between each render
	static const uint32_t DELAY_PER_UPDATE_FRAME = 1000 / TARGET_UPS; ///< milliseconds between each update

private:
	bool _isInitialized;
	bool _terminateRequested;		///< true if the GameEngine should deinitialize and shutdown
	uint32_t _updateTimeout;		///< Timestamp when updateOneFrame() should be run again
	uint32_t _renderTimeout;		///< Timestamp when renderOneFrame() should be run again
};