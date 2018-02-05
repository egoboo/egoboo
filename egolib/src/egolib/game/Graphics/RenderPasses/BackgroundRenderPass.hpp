#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// The render pass for the world background.
struct BackgroundRenderPass : public RenderPass
{
public:
    BackgroundRenderPass();
protected:
    /// A vertex type used by this render pass.
    struct Vertex
    {
        float x, y, z;
        float s, t;
    };
    /// A vertex descriptor & a vertex buffer used by this render pass.
    VertexDescriptor _vertexDescriptor;
    VertexBuffer _vertexBuffer;
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};

} // namespace Graphics
} // namespace Ego
