#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego::Graphics {
	
/// @brief Render pass for non-opaque entities.
struct NonOpaqueEntitiesRenderPass : public RenderPass
{
public:
    NonOpaqueEntitiesRenderPass();
protected:
	void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};
	
} // namespace Ego::Graphics
