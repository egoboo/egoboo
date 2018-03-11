#pragma once

#include "egolib/Graphics/GraphicsWindow.hpp"
#include "egolib/integrations/math.hpp"

namespace Ego {
namespace SDL {

/// @brief An SDL/OpenGL graphics window.
class GraphicsWindow : public Ego::GraphicsWindow
{
private:
    /// @brief A pointer to the SDL window.
    SDL_Window *window;

    /// @brief The SDL display index of the SDL window.
    /// @remark The display index is POLLED when updating the window.
    int displayIndex;

public:
    /// @brief Construct this graphics window with the specified window properties.
    /// @throw idlib::runtime_error window creation failed
    GraphicsWindow();

    /// @brief Destruct this graphics window.
    ~GraphicsWindow();

public:
    /// @copydoc idlib::window::title(const std::string&)
    void title(const std::string& title) override;

    /// @copydoc idlib::window::title()
    std::string title() const override;

    /// @copydoc idlib::window::grab_enabled(bool)
    void grab_enabled(bool enabled) override;

    /// @copydoc idlib::window::grab_enabled()
    bool grab_enabled() const override;

    /// @copydoc idlib::window::size()
    Ego::Vector2f size() const override;

    /// @copydoc idlib::window::size(const Vector2f&)
    void size(const Ego::Vector2f& size) override;

    /// @copydoc idlib::window::position()
    virtual Ego::Point2f position() const override;

    /// @copydoc Ego::GraphicsWindow::setPosition
    virtual void position(const Ego::Point2f& position) override;

    /// @copydoc idlib::window::center()
    void center() override;

    /// @copydoc idlib::window::drawable_size()
    Ego::Vector2f drawable_size() const override;

    /// @copydoc idlib::window::update()
    void update() override;

    /// @copydoc Ego::GraphicsWindow::setIcon
    void setIcon(SDL_Surface *icon) override;

    /// @copydoc Ego::GraphicsWindow::get
    SDL_Window *get() override;

    /// @copydoc Ego::GraphicsWindow::getDisplayIndex
    int getDisplayIndex() const override;

    /// @copydoc Ego::GraphicsWindow::getContents
    std::shared_ptr<SDL_Surface> getContents() const override;
}; // class GraphicsWindow

} // namespace SDL
} // namespace Ego
