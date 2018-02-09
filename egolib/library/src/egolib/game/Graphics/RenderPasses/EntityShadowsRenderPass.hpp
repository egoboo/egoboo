#pragma once

#include "egolib/game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

/// The render pass for entity shadows.
struct EntityShadowsRenderPass : public RenderPass
{
public:
    EntityShadowsRenderPass();
protected:
    void doRun(::Camera& cam, const TileList& tl, const EntityList& el) override;
private:
    /// A vertex type used by this render pass.
    struct Vertex
    {
        float x, y, z;
        float s, t;
    };
    /// A vertex descriptor & a vertex buffer used by this render pass.
    idlib::vertex_descriptor _vertexDescriptor;
    std::shared_ptr<idlib::vertex_buffer> _vertexBuffer;
    // Used if low-quality shadows are enabled.
    void doLowQualityShadow(const ObjectRef character);
    // Used if high-quality shadows are enabled.
    void doHighQualityShadow(const ObjectRef character);
    // Used by all shadow qualities.
    void doShadowSprite(float intensity, idlib::vertex_buffer& vertexBuffer, idlib::vertex_descriptor& vertexDescriptor);
};

} // namespace Graphics
} // namespace Ego
