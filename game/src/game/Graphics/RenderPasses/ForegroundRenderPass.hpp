#pragma once

#include "game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// The render pass for the world foreground.
struct ForegroundRenderPass : public RenderPass
{
public:
    ForegroundRenderPass();
protected:
    /// A vertex type used by this render pass.
    struct Vertex
    {
        float x, y, z;
        float s, t;
    };
    /// A vertex buffer used by this render pass.
    VertexBuffer _vertexBuffer;
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};


} // namespace Graphics
} // namespace Ego
