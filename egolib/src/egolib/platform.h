//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/platform.h
/// @brief System-dependent global parameters.
///   @todo  some of this stuff is compiler dependent, rather than system dependent.

#pragma once

#include "IdLib/IdLib.hpp"

#if defined(ID_IOS) || defined(ID_IOSSIMULATOR)
    #error iOS or iOS simulator are not yet supported
#endif

/// Define __EGO_CURRENT_FILE__, __EGO_CURRENT_LINE__ and __EGO_CURRENT_FUNCTION__.
/// Those constants will either be properly defined or not at all.
#include "egolib/Platform/CurrentFunction.inline"

//--------------------------------------------------------------------------------------------
// SDL.
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>


#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// OSX definitions

#if defined(ID_OSX)
    /// Make this function work cross-platform.
    #define stricmp strcasecmp
#endif

#if defined(ID_LINUX)
    /// Make this function work cross-platform.
    /// @todo Detect POSIX version if this function is available at all.
    #define stricmp strcasecmp
#endif

//--------------------------------------------------------------------------------------------
// windows definitions

#if defined(ID_WINDOWS)
    /// Speeds up compile times a bit.  We don't need everything in windows.h.
    /// @todo MH: Nice, except of that system headers like windows.h should not
    ///       be included in general code at all.
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN

    /// Special win32 macro that lets windows know that you are going to be
    /// starting from a console. This is useful because you can get real-time
    /// output to the screen just by using printf()!
    #if defined(_CONSOLE)
        #define CONSOLE_MODE
    #else
        #undef CONSOLE_MODE
    #endif

#endif

//--------------------------------------------------------------------------------------------

// os-dependent pathname conventions

#define C_SLASH_CHR     '/'
#define C_SLASH_STR     "/"

#define C_BACKSLASH_CHR '\\'
#define C_BACKSLASH_STR "\\"

#define WIN32_SLASH_CHR C_BACKSLASH_CHR
#define WIN32_SLASH_STR C_BACKSLASH_STR

// everyone uses the same convention for the internet...
#define NET_SLASH_CHR C_SLASH_CHR
#define NET_SLASH_STR C_SLASH_STR

#if defined(ID_WINDOWS)
    #define SLASH_STR WIN32_SLASH_STR
    #define SLASH_CHR WIN32_SLASH_CHR
#else
    #define SLASH_STR NET_SLASH_STR
    #define SLASH_CHR NET_SLASH_CHR
#endif

//--------------------------------------------------------------------------------------------

// Compiler-specific definitions

// MSVC does not support noexcept yet.
#if defined(_MSC_VER)
    #define EGO_NOEXCEPT throw()
#else
    #define EGO_NOEXCEPT noexcept
#endif

//--------------------------------------------------------------------------------------------
// Turn off warnings that we don't care about.
/// @todo MH: This should be reviewed.

#if defined(_MSC_VER)
    #pragma warning(disable : 4090) ///< '=' : different 'const' qualifiers (totally unimportant in C)
    #pragma warning(disable : 4200) ///< zero-sized array in struct/union (used in the md2 loader)
    #pragma warning(disable : 4201) ///< nameless struct/union (nameless unions and nameless structs used in defining the vector structs)
    #pragma warning(disable : 4204) ///< non-constant aggregate initializer (used to simplify some vector initializations)
    #pragma warning(disable : 4244) ///< conversion from 'double' to 'float'
    #pragma warning(disable : 4305) ///< truncation from 'double' to 'float'

    #if !defined(_DEBUG)
        #pragma warning(disable : 4554) ///< possibly operator precendence error
    #endif
#endif

//--------------------------------------------------------------------------------------------

#if !defined(SET_PACKED)
    // Set the packing of a data structure at declaration time.
    #if !defined(USE_PACKING)
        // Do not actually do anything about the packing.
        #define SET_PACKED()
    #else
    // Use compiler-specific macro definitions.
    #if defined(__GNUC__)
        #define SET_PACKED() __attribute__ ((__packed__))
    #elif defined(_MSC_VER)
        #define SET_PACKED()
    #endif
#endif
#endif
    
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------

namespace Ego
{
namespace Core
{

/**
 * @brief
 *  If defined to @a 1,
 *  the singleton inherits from Ego::Core::NonCopyable.
 */
#define EGO_CORE_SINGLETON_NONCOPYABLE (1)

template <typename InstanceType,typename FactoryType>
class _BasicSingleton
{
protected:
    /// @brief Constructor.
    /// @remark Intentionally protected.
    _BasicSingleton() { }

    /// @brief Destructor.
    /// @remark Intentionally protected.
    virtual ~_BasicSingleton() { }

protected:
    /// @brief Mutex protecting non-atomic operations.
    /// @remark Intentionally private.
    static std::mutex _mutex;

    /// @brief Singleton instance.
    /// @remark Intentionally private.
    static std::atomic<InstanceType *> _instance;

public:

    /**
    * @warning
    *  The test is just informative, its information is
    *  not imperative - by the time the caller acquires
    *  the information, facts might already have changed.
    */
    static bool isInitialized() {
        InstanceType *t = _instance.load();
        return nullptr != t;
    }

    /**
    * @brief
    *  Get the instance.
    * @return
    *  the instance
    * @pre
    *  The singleton must be initialized.
    * @warning
    *  Uninitializing the singleton will invalidate any references returned by calls to this method prior to uninitialization.
    */
    static InstanceType& get() {
        InstanceType *t = _instance.load();
        if (!t) {
            throw std::logic_error("singleton not initialized");
        }
        return *t;
    }

    /**
    * @brief
    *  Initialize the singleton.
    * @post
    *  The singleton was initialized if no exception was raised by this call.
    * @remark
    *  If the singleton is initialized, a call to this method is a no-op.
    */
    static void initialize()
    {
        InstanceType *t = _instance.load();
        if (!t) { // 1st check.
            std::lock_guard<std::mutex> lock(_mutex);
            t = _instance.load();
            if (!t) { // 2nd check.
                t = FactoryType()();
                _instance.store(t);
            }
        }
    }

    /**
    * @brief
    *  Uninitialize the singleton.
    * @remark
    *  If the singleton is not initialized, a call to this method is a no-op.
    */
    static void uninitialize() {
        InstanceType *t = _instance.load();
        if (t) { // 1st check.
            std::lock_guard<std::mutex> lock(_mutex); // 2nd check.
            t = _instance.load();
            if (t) {
                delete t;
                _instance.store(nullptr);
            }
        }
    }
};

/**
 * @brief
 *  Double-checked locking singleton.
 *  The specializations <tt>Singleton&lt;InstanceType&gt;</tt> and
 *  <tt>singleton&lt;InstanceType,typename FactoryType&gt;</tt>are
 *  provided. The former creates a singleton of type @a InstanceType
 *  while the latter creates the singleton via a factory @a FactoryType.
 *  @a Factory must be a <tt>struct</tt> or <tt>class</tt> which
 *  provides a <tt>operator()</tt> that takes no arguments and
 *  returns an @a InstanceType or derived pointer.
 *
 *  Furthermore, <tt>Singleton</tt> inherits from Ego::Core::NonCopyable
 *  and prevents copy-construction and copy-assignment of derived classes.
 *
 *  Example usage for <tt>Singleton&lt;InstanceType&gt;</tt>:
 *  @code
 *  class A : public Singleton<B> {};
 *  @endcode
 *
 *  Example usage for <tt>Singleton&lt;InstanceType,typename FactoryType&gt;</tt>:
 *  @code
 *  class X { }
 *  class Y : X { }
 *  class Z : Y { }
 *  struct F { Y *operator()() { new Z(); } }
 *  class A : Singleton<X,F> {};
 * @author
 *  Michael Heilmann
 * @see
 *  Myers, Alexadrescu; "C++ and the Perils of Double-Checked Locking"; 2014
 */
template <typename ... Dummy>
class Singleton;


template <typename InstanceType>
class Singleton<InstanceType>
#if defined(EGO_CORE_SINGLETON_NONCOPYABLE) && 1 == EGO_CORE_SINGLETON_NONCOPYABLE
    : Id::NonCopyable
#endif
{

protected:
    /// @brief Constructor.
    /// @remark Intentionally protected.
    Singleton() { }

    /// @brief Destructor.
    /// @remark Intentionally protected.
    virtual ~Singleton() { }

private:

    /// @brief Mutex protecting non-atomic operations.
    /// @remark Intentionally private.
    static std::mutex _mutex;

    /// @brief Singleton instance.
    /// @remark Intentionally private.
    static std::atomic<InstanceType *> _instance;

public:

    /**
     * @warning
     *  The test is just informative, its information is
     *  not imperative - by the time the caller acquires
     *  the information, facts might already have changed.
     */
    static bool isInitialized() {
        InstanceType *t = _instance.load();
        return nullptr != t;
    }

    /**
     * @brief
     *  Get the instance.
     * @return
     *  the instance
     * @pre
     *  The singleton must be initialized.
     * @warning
     *  Uninitializing the singleton will invalidate any references returned by calls to this method prior to uninitialization.
     */
    static InstanceType& get() {
        InstanceType *t = _instance.load();
        if (!t) {
            throw std::logic_error("singleton not initialized");
        }
        return *t;
    }

    /**
     * @brief
     *  Initialize the singleton.
     * @post
     *  The singleton was initialized if no exception was raised by this call.
     * @remark
     *  If the singleton is initialized, a call to this method is a no-op.
     */
    static void initialize()
    {
        InstanceType *t = _instance.load();
        if (!t) { // 1st check.
            std::lock_guard<std::mutex> lock(_mutex);
            t = _instance.load();
            if (!t) { // 2nd check.
                t = new InstanceType();
                _instance.store(t);
            }
        }
    }

    /**
     * @brief
     *  Uninitialize the singleton.
     * @remark
     *  If the singleton is not initialized, a call to this method is a no-op.
     */
    static void uninitialize() {
        InstanceType *t = _instance.load();
        if (t) { // 1st check.
            std::lock_guard<std::mutex> lock(_mutex); // 2nd check.
            t = _instance.load();
            if (t) {
                delete t;
                _instance.store(nullptr);
            }
        }
    }

};

template <typename InstanceType, typename FactoryType>
class Singleton<InstanceType, FactoryType>
#if defined(EGO_CORE_SINGLETON_NONCOPYABLE) && 1 == EGO_CORE_SINGLETON_NONCOPYABLE
    : Id::NonCopyable
#endif
{

protected:
    /// @brief Constructor.
    /// @remark Intentionally protected.
    Singleton() { }

    /// @brief Destructor.
    /// @remark Intentionally protected.
    virtual ~Singleton() { }

private:
    /// @brief Mutex protecting non-atomic operations.
    /// @remark Intentionally private.
    static std::mutex _mutex;

    /// @brief Singleton instance.
    /// @remark Intentionally private.
    static std::atomic<InstanceType *> _instance;

public:

    /**
     * @warning
     *  The test is just informative, its information is
     *  not imperative - by the time the caller acquires
     *  the information, facts might already have changed.
     */
    static bool isInitialized() {
        InstanceType *t = _instance.load();
        return nullptr != t;
    }

    /**
     * @brief
     *  Get the instance.
     * @return
     *  the instance
     * @pre
     *  The singleton must be initialized.
     * @warning
     *  Uninitializing the singleton will invalidate any references returned by calls to this method prior to uninitialization.
     */
    static InstanceType& get() {
        InstanceType *t = _instance.load();
        if (!t) {
            throw std::logic_error("singleton not initialized");
        }
        return *t;
    }

    /**
     * @brief
     *  Initialize the singleton.
     * @post
     *  The singleton was initialized if no exception was raised by this call.
     * @remark
     *  If the singleton is initialized, a call to this method is a no-op.
     */
    static void initialize()
    {
        InstanceType *t = _instance.load();
        if (!t) { // 1st check.
            std::lock_guard<std::mutex> lock(_mutex);
            t = _instance.load();
            if (!t) { // 2nd check.
                t = FactoryType()();
                _instance.store(t);
            }
        }
    }

    /**
     * @brief
     *  Uninitialize the singleton.
     * @remark
     *  If the singleton is not initialized, a call to this method is a no-op.
     */
    static void uninitialize() {
        InstanceType *t = _instance.load();
        if (t) { // 1st check.
            std::lock_guard<std::mutex> lock(_mutex); // 2nd check.
            t = _instance.load();
            if (t) {
                delete t;
                _instance.store(nullptr);
            }
        }
    }

};

template <typename InstanceType>
std::mutex Singleton<InstanceType>::_mutex;

template <typename InstanceType>
std::atomic<InstanceType *> Singleton<InstanceType>::_instance(nullptr);

template <typename InstanceType, typename FactoryType>
std::mutex Singleton<InstanceType, FactoryType>::_mutex;

template <typename InstanceType, typename FactoryType>
std::atomic<InstanceType *> Singleton<InstanceType, FactoryType>::_instance(nullptr);

} // namespace Core
} // namespace Ego
