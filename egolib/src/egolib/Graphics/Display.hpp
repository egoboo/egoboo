#pragma once

#include "egolib/platform.h"

namespace Ego {

// Forward declaration.
class DisplayMode;

/// @brief A display.
class Display
{
protected:
    /// @brief List of display modes of this display.
    std::vector<std::shared_ptr<DisplayMode>> displayModes;

protected:
    /// @brief Construct this display.
    /// @remark Intentionally protected.
    Display();

    /// @brief Destruct this display.
    /// @remark Intentionally protected.
    virtual ~Display();

protected:
    /// @brief Displays can not be copy-constructed.
    /// @remark Intentionally protected.
    Display(const Display&) = delete;

    /// @brief Displays can not be assigned.
    /// @remark Intentionally protected.
    const Display& operator=(const Display&) = delete;

public:
    /// @brief Compare this display with another display for equality.
    /// @param other the other display
    /// @return @a true if this display is equal to the other display, @a false otherwise
    bool operator==(const Display& other) const;

    /// @brief Compare this display with another display mode for inequality.
    /// @param other the other display
    /// @return @a true if this display is not equal to the other display, @a false otherwise
    bool operator!=(const Display& other) const;

protected:
    virtual bool compare(const Display&) const = 0;

public:
    /// @brief Get if this display is the primary display.
    /// @return @a true if this display the primary display, @a false otherwise
    virtual bool isPrimaryDisplay() const = 0;
    
    /// @brief Get the display index of this display.
    /// @return the display index of this display
    virtual int getDisplayIndex() const = 0;

    /// @brief Get the display modes of this display.
    /// @return the display modes of this display
    const std::vector<std::shared_ptr<DisplayMode>>& getDisplayModes() const;

    /// @brief Update this display.
    virtual void update() = 0;
};

} // namespace Ego
