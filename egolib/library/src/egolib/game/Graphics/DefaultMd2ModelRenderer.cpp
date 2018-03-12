#include "egolib/game/Graphics/DefaultMd2ModelRenderer.hpp"
#include "egolib/Graphics/MD2Model.hpp"
#include "egolib/Graphics/VertexFormat.hpp"

namespace Ego::Graphics {

DefaultMd2ModelRenderer::DefaultMd2ModelRenderer()
    : m_vertices(new Vertex[0]), m_size(0),
      vertexDescriptor(descriptor_factory<idlib::vertex_format::P3FC4FT2FN3F>()())
{}

DefaultMd2ModelRenderer::~DefaultMd2ModelRenderer()
{
    delete[] m_vertices;
    m_vertices = nullptr;
    m_size = 0;
}

void *DefaultMd2ModelRenderer::lock()
{
    return m_vertices;
}

void DefaultMd2ModelRenderer::ensureSize(size_t requiredSize)
{
    if (requiredSize <= m_size)
    {
        return;
    }
    auto newSize = requiredSize;
    auto newVertices = new Vertex[requiredSize];
    memcpy(newVertices, m_vertices, m_size);
    delete[] m_vertices;
    m_vertices = newVertices;
    m_size = newSize;
}


size_t DefaultMd2ModelRenderer::getRequiredVertexBufferCapacity(const MD2Model& model) const
{
    size_t capacity = 0;
    for (const MD2_GLCommand& glcommand : model.getGLCommands())
    {
        capacity = std::max(capacity, glcommand.data.size());
    }
    return capacity;
}

} // namespace Ego::Graphics
