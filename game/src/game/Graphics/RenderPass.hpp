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

/// @file game/Graphics/RenderPass.hpp
/// @brief Abstract implementation render passes
/// @author Michael Heilmann

#pragma once

#include "game/Graphics/Camera.hpp"
#include "game/Graphics/TileList.hpp"
#include "game/Graphics/EntityList.hpp"

namespace Ego {
namespace Graphics {
/**
 * @brief
 *	An abstract render pass.
 */
struct RenderPass {
public:
	// To shorten expressions.
	template <typename _ClockPolicy>
	using Clock = Ego::Time::Clock<_ClockPolicy>;
	// To shorten expressions.
	template <typename _ClockPolicy>
	using ClockScope = Ego::Time::ClockScope <_ClockPolicy>;
	// To shorten expressions.
	using ClockPolicy = Ego::Time::ClockPolicy;
	/**
	 * @brief
	 *	The clock for measuring the time spent in this render pass.
	 */
	Clock<ClockPolicy::NonRecursive> _clock;
	/**
	 * @brief
	 *	Construct this render pass.
	 * @param name
	 *	the name of this render pass. Names of render passes are pairwise different
	 * @remark
	 *	Intentionally protected.
	 */
	RenderPass(const std::string& name)
		: _clock(name, 512) {
	}
	/**
	 * @brief
	 *	Destruct this render pass.
	 * @remark
	 *	Intentionally protected.
	 */
	virtual ~RenderPass() {
	}
	/**
	 * @brief
	 *	Perform the rendering.
	 * @param camera
	 *	the camera to be used
	 * @param tileList
	 *	the tile list to be used
	 * @param entityList
	 *	the entity list to be used
	 */
	virtual void doRun(::Camera& camera, const Ego::Graphics::TileList& tileList, const EntityList& entityList) = 0;
public:
	/**
	 * @brief
	 *	Run this pass.
	 * @param camera
	 *	the camera to be used
	 * @param tileList
	 *	the render list to be used
	 * @param entityList
	 *	the entity list to be used
	 */
	void run(::Camera& camera, const Ego::Graphics::TileList& tileList, const EntityList& entityList) {
		ClockScope<ClockPolicy::NonRecursive> clockScope(_clock);
		Ego::OpenGL::Utilities::isError();
		doRun(camera, tileList, entityList);
		Ego::OpenGL::Utilities::isError();
	}

};

} // namespace Graphics
} // namespace Ego
