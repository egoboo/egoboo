#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// The render pass for non-reflective tiles
/// i.e. tiles which do not reflect entities.
struct NonReflectiveTilesRenderPass : public RenderPass
{
public:
    NonReflectiveTilesRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};


} // namespace Graphics
} // namespace Ego
