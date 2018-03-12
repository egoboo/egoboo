#pragma once

#include "egolib/Graphics/GraphicsSystemNew.hpp"

namespace Ego::SDL {

// Forward declaration.
class GraphicsWindow;

class GraphicsSystemNew : public Ego::GraphicsSystemNew
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

    /** @copydoc Ego::GraphicsSystemNew::createContext */
    Ego::GraphicsContext *createContext(Ego::GraphicsWindow *window) override;

    /** @copydoc Ego::GraphicsSystemNew::createWindow */
    Ego::GraphicsWindow *createWindow() override;

};

} // namespace Ego::SDL
