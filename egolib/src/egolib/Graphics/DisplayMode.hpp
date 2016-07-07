#pragma once

#include "egolib/platform.h"

namespace Ego {
/// A display mode.
struct DisplayMode {
protected:
    /// Construct this display mode.
	/// @remark Intentionally protected.
    DisplayMode();

    /// Destruct this display mode.
	/// @remark Intentionally protected.
    virtual ~DisplayMode();

protected:
    /// Display modes can not be copy-constructed.
	/// @remark Intentionally protected.
    DisplayMode(const DisplayMode&) = delete;

    /// Display modes can not be assigned.
	/// @remark Intentionally protected.
    const DisplayMode& operator=(const DisplayMode&) = delete;

public:
    /// Compare this display mode with another display mode for equality.
    /// @param other the other display mode
    /// @return @a true if this display mode is equal to the other display mode, @a false otherwise
    bool operator==(const DisplayMode& other) const;

    /// Coompare this display mode with another display mode for inequality.
    /// @param other the other display mode
    /// @return @a true if this display mode is not equal to the other display mode, @a false otherwise
    bool operator!=(const DisplayMode& other) const;

protected:
    virtual bool compare(const DisplayMode&) const = 0;

public:
    /// Get a pointer to the backend display mode.
    /// @return a pointer to the backend display mode
    virtual void *get() const = 0;

    /// Get the horizontal resolution, in pixels, of this display mode.
    /// @return the horizontal resolution, in pixels, of this display mode.
    virtual int getHorizontalResolution() const = 0;

    /// Get the vertical resolution, in pixels, of this display mode.
    /// @return the vertical resolution, in pixels, of this display mode.
    virtual int getVerticalResolution() const = 0;

    /// Get the refresh rate, in Hertz, of this display mode.
    /// @return the refresh rate, in Hertz, of this display mode
    virtual int getRefreshRate() const = 0;
};
	
} // namespace Ego
