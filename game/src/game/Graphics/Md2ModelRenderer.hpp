#pragma once

#include "game/egoboo.h"

// Forward declaration.
class MD2Model;

namespace Ego {
namespace Graphics {

/// @brief Abstract renderer for MD2 models.
class Md2ModelRenderer
{
protected:
    /// @brief Construct this abstract MD2 model renderer.
    /// @remark Intentionally protected.
    Md2ModelRenderer();

public:
    /// @brief Destruct this MD2 model renderer.
    virtual ~Md2ModelRenderer();

    /// @brief Ensure the size of the vertex buffer is greater than or equal to a required size.
    /// @param requiredSize the required size
    virtual void ensureSize(size_t requiredSize) = 0;

    /// @brief Compute the required vertex buffer capacity for a model.
    /// @param model the model
    /// @return the required vertex buffer capacity
    virtual size_t getRequiredVertexBufferCapacity(const MD2Model& model) const = 0;

    virtual void *lock() = 0;

}; // class M2dModelRenderer

} // namespace Graphics
} // namespace Ego

