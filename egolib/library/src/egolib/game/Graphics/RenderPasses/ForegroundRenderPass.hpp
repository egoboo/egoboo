#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

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
    /// A vertex descriptor & a vertex buffer used by this render pass.
    idlib::vertex_descriptor _vertexDescriptor;
    std::shared_ptr<idlib::vertex_buffer> _vertexBuffer;
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
};


} // namespace Graphics
} // namespace Ego
