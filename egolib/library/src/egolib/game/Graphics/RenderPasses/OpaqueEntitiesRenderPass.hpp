#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego::Graphics {
	
/// @brief Render pass for opaque entities.
struct OpaqueEntitiesRenderPass : public RenderPass
{
public:
	OpaqueEntitiesRenderPass();
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};
	
} // namespace Ego::Graphics
