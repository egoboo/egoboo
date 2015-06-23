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
	/**
	 * @brief
	 *	The clock for measuring the time spent in this render pass.
	 */
	ClockState_t _clock;
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
	virtual void doRun(Camera& camera, const Ego::Graphics::TileList& tileList, const EntityList& entityList) = 0;
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
	void run(Camera& camera, const Ego::Graphics::TileList& tileList, const EntityList& entityList) {
		ClockScope clockScope(_clock);
		Ego::OpenGL::Utilities::isError();
		doRun(camera, tileList, entityList);
		Ego::OpenGL::Utilities::isError();
	}

};

} // namespace Graphics
} // namespace Ego
