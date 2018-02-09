#pragma once

#include "egolib/game/Graphics/Md2ModelRenderer.hpp"

namespace Ego {
namespace Graphics {

/// @brief Default renderer for MD2 models.
class DefaultMd2ModelRenderer : public Md2ModelRenderer
{
public:
    // Forward declaration.
    struct Vertex;
protected:
    /// @brief The vertex descriptor.
    idlib::vertex_descriptor vertexDescriptor;

    /// @brief The size of the vertex buffer.
    size_t m_size;

    /// @brief The vertices of the vertex buffer-
    Vertex *m_vertices;

public:
    /// @brief Construct this MD2 model renderer.
    DefaultMd2ModelRenderer();

    /// @brief Destruct this MD2 model renderer.
    virtual ~DefaultMd2ModelRenderer();

    /// @copydoc Md2ModelRenderer::ensureSize
    void ensureSize(size_t requiredSize) override;

    /// @copydoc Md2ModelRenderer::getRequiredVertexBufferCapacity
    size_t getRequiredVertexBufferCapacity(const MD2Model& model) const override;

    /// @copydoc Md2ModelRenderer::lock()
    void *lock() override;

    /// @brief A vertex.
    struct Vertex
    {
        struct
        {
            float x, y, z;
        } position;
        struct
        {
            float r, g, b, a;
        } colour;
        struct
        {
            float s, t;
        } texture;
        struct
        {
            float x, y, z;
        } normal;
    };

    /// @brief List of vertices to be rendered.
    std::vector<Vertex> vertices;

}; // class DefaultMd2ModelRenderer

} // namespace Graphics
} // namespace Ego
