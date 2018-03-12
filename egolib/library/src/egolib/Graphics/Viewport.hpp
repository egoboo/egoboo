#pragma once

#include "egolib/integrations/video.hpp"
#include "idlib/signal.hpp"

namespace Ego::Graphics {

/// @brief A viewport.
class Viewport : public idlib::viewport
{
private:
    /// @brief The rectangle, in pixels, of this viewport.
    idlib::rectangle_2s m_absolute_rectangle;

public:
    /// @brief Construct this viewport with default values.
    Viewport();
    
    /// @brief Destruct this viewport.
    virtual ~Viewport();

public:
    /// @brief Signal raised if the absolute rectangle of the viewport changed.
    /// The first argument is the old rectangle, the second argument is the new rectangle.
    idlib::signal<void(idlib::rectangle_2s, idlib::rectangle_2s)> absolute_rectangle_changed;

    /// @brief Get the rectangle, in pixels, of this viewport.
    /// @return the rectangle
    /// @default The default value is \f$(0, 0, 800, 600)\f$.
    idlib::rectangle_2s absolute_rectangle() const;
    
    /// @brief Set the rectangle, in pixels, of this viewport.
    /// @param absolute_rectangle the rectangle
    void absolute_rectangle(const idlib::rectangle_2s& absolute_rectangle);
};

} // namespace Ego::Graphics
