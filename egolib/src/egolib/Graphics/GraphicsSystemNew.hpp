#pragma once

#include "egolib/Core/Singleton.hpp"
#include "egolib/Log/_Include.hpp"

namespace Ego {

// Forward declaration.
class GraphicsSystemNew;
class Display;
class GraphicsWindow;
class GraphicsContext;
class WindowProperties;

namespace Core {

/// @brief Creator functor creating the back-end.
template <>
struct CreateFunctor<GraphicsSystemNew>
{
    GraphicsSystemNew *operator()() const;
};

} // namespace Core


class GraphicsSystemNew : public Core::Singleton<GraphicsSystemNew>
{
protected:
    friend Core::Singleton<GraphicsSystemNew>::CreateFunctorType;
    friend Core::Singleton<GraphicsSystemNew>::DestroyFunctorType;

protected:
    /// @brief List of displays.
    std::vector<std::shared_ptr<Display>> displays;

    /// @brief The driver name.
    std::string driverName;

protected:
    /// @brief Construct this graphics system.
    GraphicsSystemNew();

    /// @brief Destruct this graphics system.
    virtual ~GraphicsSystemNew();

public:
    /// @brief Set the cursor visibility.
    /// @param visibility @a true shows the cursor, @a false hides the cursor
    /// @throw id::environment_error the environment failed
    virtual void setCursorVisibility(bool visibility) = 0;

    /// @brief Get the cursor visibility.
    /// @return @a true if the cursor is shown, @a false otherwise
    /// @throw id::environment_error the environment failed
    virtual bool getCursorVisibility() const = 0;

    /// @brief Get the list of displays.
    /// @return the list of displays
    const std::vector<std::shared_ptr<Display>>& getDisplays() const;

    /// @brief Get the driver name.
    /// @return the driver name
    const std::string& getDriverName() const;

    /// @brief Update this graphics system.
    virtual void update() = 0;

    /// @brief Create a graphics context.
    /// @param window a pointer to a window
    /// @return a pointer to the graphics context on success, a null pointer on failure
    virtual GraphicsContext *createContext(GraphicsWindow *window) = 0;

    /// @hrief Create a graphics window.
    /// @return a pointer to the graphics window on success, a null pointer on failure
    virtual GraphicsWindow *createWindow() = 0;

};

} // namespace Ego

Log::Entry& operator<<(Log::Entry& logEntry, const Ego::GraphicsSystemNew& graphicsSystem);
