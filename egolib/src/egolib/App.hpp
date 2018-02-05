#pragma once

#include "egolib/platform.h"

namespace Ego {

/// @brief The PIMPL implementation.
struct AppImpl final
{
public:
    /// @brief Construct this implementation.
    /// @param title the application title
    /// @param version the application version
    AppImpl(const std::string& title, const std::string& version);

    /// @brief Destruct this implementation.
    ~AppImpl();
};

/// @brief Base of all applications.
/// Derive from this class for your own game, editor, tool, ...
/// The App class has lots of dependencies and uses the PIMPL pattern to avoid including headers into this header file.
template <typename T>
struct App : public idlib::singleton<T>
{
private:
    /// @brief The PIMPL pointer.
    std::unique_ptr<AppImpl> impl;

protected:
    /// @brief Construct this application.
    /// @param title the application title
    /// @param version the application version
    App(const std::string& title, const std::string& version) :
        impl(std::make_unique<AppImpl>(title, version))
    {}

    /// @brief Destruct this application.
    virtual ~App()
    {}
};

} // namespace Ego
