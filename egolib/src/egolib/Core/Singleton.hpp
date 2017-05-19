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

/// @file   egolib/Core/Singleton.hpp
/// @brief  Singleton implementation
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Core {

/**
 * @brief Create an object of type @a Type.
 */
template <typename Type, typename ... ArgumentTypes>
struct CreateFunctor {
    Type *operator()(ArgumentTypes&& ... arguments) const {
        return new Type(std::forward<ArgumentTypes>(arguments)...);
    }
};

/**
 * @brief Destroy an object of type @a Type.
 */
template <typename Type>
struct DestroyFunctor {
    void operator()(Type *o) const {
        delete o;
    }
};

/**
 * @brief
 *  If defined to @a 1,
 *  the singleton inherits from Ego::Core::NonCopyable.
 */
#define EGO_CORE_SINGLETON_NONCOPYABLE (1)

/**
 * @brief
 *  Double-checked locking singleton.
 *  The specializations <tt>Singleton&lt;InstanceType&gt;</tt> and
 *  creates a singleton. For that, a @a CreateFunctor and a @a DestroyFunctor
 *  need to be provided. 
 *
 *  Furthermore, <tt>Singleton</tt> inherits from Ego::Core::NonCopyable
 *  and prevents copy-construction and copy-assignment of derived classes.
 *
 *  Example usage for <tt>Singleton&lt;InstanceType&gt;</tt>:
 *  @code
 *  class A : public Singleton<B> {};
 *  @endcode
 *
 *  The following example demonstrates how to create create a singleton for an audio device:
 *  @code
 *
 *  template <> struct CreateFunctor<ConcreteAudioDevice> {
 *    AudioDevice *operator()() const {
 *      new ConcreteAudioDevice();
 *    } 
 *  };
 *
 *  template <> struct DestroyFunctor<ConcreteAudioDevice> {
 *    void operator()(AudioDevice *o) const {
 *      delete o; 
 *    }
 *  };
 *
 *  class AudioDevice : public Singleton<Device,CreateFunctor<ConcreteAudioDevice>,DestroyFunctor<ConcreteAudioDevice>> {
 *  protected:
 *    using MyCreateFunctor = CreateFunctor<ConcreteAudioDevice>;
 *    friend MyCreateFunctor;
 *    using MyDestroyFunctor = DestroyFunctor<ConcreteAudioDevice>;
 *    using MyDestroyFunctor;
 *  protected:
 *    AudioDevice() {}
 *    virtual ~AudioDevice() {}
 *  };
 *  
 *  class AbstractAudioDevice : public AudioDevice {
 *  protected:
 *    Y() {}
 *    virtual ~Y() {} 
 *  };
 *
 *  class ConcreteAudioDevice : public AbstractAudioDevice {
 *  protected:
 *    ConcreteAudioDevice() {}
 *    virtual ~ConcreteAudioDevice() {}
 *  };
 *
 *  @endcode
 * @author
 *  Michael Heilmann
 * @see
 *  Myers, Alexadrescu; "C++ and the Perils of Double-Checked Locking"; 2014
 */
template < typename InstanceType, typename Type = InstanceType>
class Singleton
#if defined(EGO_CORE_SINGLETON_NONCOPYABLE) && 1 == EGO_CORE_SINGLETON_NONCOPYABLE
    : private id::non_copyable
#endif
{
public:
    using CreateFunctorType = CreateFunctor<Type>;
    using DestroyFunctorType = DestroyFunctor<Type>;
protected:
    /// @brief Constructor.
    /// @remark Intentionally protected.
    Singleton() {}

    /// @brief Destructor.
    /// @remark Intentionally protected.
    virtual ~Singleton() {}

protected:
    /// @brief Mutex protecting non-atomic operations.
    /// @remark Intentionally private.
    static std::mutex mutex;

    /// @brief Singleton instance.
    /// @remark Intentionally private.
    static std::atomic<InstanceType *> instance;

public:
    /**
     * @warning
     *  The test is just informative, its information is
     *  not imperative - by the time the caller acquires
     *  the information, facts might already have changed.
     */
    static bool isInitialized() {
        InstanceType *o = instance.load();
        return nullptr != o;
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
        InstanceType *o = instance.load();
        if (!o) {
            throw std::logic_error("singleton not initialized");
        }
        return *o;
    }

    /**
     * @brief
     *  Uninitialize the singleton.
     * @remark
     *  If the singleton is not initialized, a call to this method is a no-op.
     */
    static void uninitialize() {
        InstanceType *o = instance.load();
        if (o) { // 1st check.
            std::lock_guard<std::mutex> lock(mutex); // 2nd check.
            o = instance.load();
            if (o) {
                instance.store(nullptr);
                DestroyFunctorType()(o);
            }
        }
    }

    /**
     * @brief
     *  Initialize the singleton.
     * @post
     *  The singleton was initialized if no exception was raised by this call.
     * @remark
     *  If the singleton is initialized, a call to this method is a no-op.
     */
    template<typename ... ArgumentTypes>
    static void initialize(ArgumentTypes&& ... arguments) {
        InstanceType *o = instance.load();
        if (!o) { // 1st check.
            std::lock_guard<std::mutex> lock(mutex);
            o = instance.load();
            if (!o) { // 2nd check.
                o = CreateFunctorType()(std::forward<ArgumentTypes>(arguments) ...);
                instance.store(o);
            }
        }
    }
};

template <typename InstanceType, typename Type>
std::mutex Singleton<InstanceType, Type>::mutex;

template <typename InstanceType, typename Type>
std::atomic<InstanceType *> Singleton<InstanceType, Type>::instance(nullptr);

} // namespace Core
} // namespace Ego
