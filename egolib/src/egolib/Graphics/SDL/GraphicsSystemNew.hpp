#pragma once

#include "egolib/Graphics/GraphicsSystemNew.hpp"

namespace Ego {
namespace SDL {

struct GraphicsSystemNew : public Ego::GraphicsSystemNew
{
public:
    /// @brief Construct this SDL graphics system.
    GraphicsSystemNew();

    /// @brief Destruct this SDL graphics system.
    ~GraphicsSystemNew();

    /** @copydoc Ego::GraphicsSystemNew::setCursorVisibility */
    void setCursorVisibility(bool visibility) override;

    /** @copydoc Ego::GraphicsSystemNew::getCursorVisibility */
    bool getCursorVisibility() const override;

    /** @copydoc Ego::GraphicsSystemNew::update */
    void update() override;

};

} // namespace SDL
} // namespace Ego
