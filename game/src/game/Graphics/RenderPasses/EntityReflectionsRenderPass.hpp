#pragma once

#include "game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// The render pass for entity reflections.
struct EntityReflectionsRenderPass : public RenderPass
{
public:
    EntityReflectionsRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

} // namespace Graphics
} // namespace Ego
