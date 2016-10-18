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

/// @file   egolib/Core/System.hpp
/// @brief  System services
/// @author Michael Heilmann
/// @todo   friend std::unique_ptr<T> std::make_unique<T>(); allows for an invocation of
///         constructor T::T() which is undesirable. Using the attorney-client pattern
///         might provide a solution.

#pragma once

#include "egolib/Core/Singleton.hpp"

namespace Ego {
namespace Core {

/// @brief A virtual file system, timer, an event threading service.
class SystemService : public Id::NonCopyable {
protected:
    friend class System;
    explicit SystemService(const std::string& binaryPath);
    explicit SystemService(const std::string& binaryPath, const std::string& egobooPath);
    virtual ~SystemService();
public:
    /// @brief The version of the Egoboo Engine.
    static const std::string VERSION;
    /// @brief Returns the number of milliseconds since the initialization of the timer service.
    /// @return the number of milliseconds since the initialization of the timer service.
    /// @remark This value wraps around if the program runs for more than 49.7 days. 
    uint32_t getTicks();
};

/// @brief A video service.
class VideoService : public Id::NonCopyable {
protected:
	friend class System;
	explicit VideoService();
	virtual ~VideoService();
};

/// @brief An audio service.
class AudioService : public Id::NonCopyable {
protected:
	friend class System;
    explicit AudioService();
	virtual ~AudioService();
};

/// @brief An input service.
class InputService : public Id::NonCopyable {
protected:
	friend class System;
    explicit InputService();
	virtual ~InputService();
};

class System : public Singleton<System> {
protected:
    friend Singleton<System>::CreateFunctorType;
    friend Singleton<System>::DestroyFunctorType;
    /// @brief Construct this system.
    /// @remark Intentionally protected.
    System(const std::string& binaryPath, const std::string& egobooPath);
    /// @brief Construct this system.
    /// @remark Intentionally protected.
    System(const std::string& binaryPath);

    /// @brief Destruct this system.
    /// @remark Intentionally protected.
    virtual ~System();

private:
    SystemService *systemService;
    VideoService *videoService;
    AudioService *audioService;
    InputService *inputService;
public:
    SystemService& getSystemService() {
        return *systemService;
    }
	VideoService& getVideoService() {
		return *videoService;
	}
	AudioService& getAudioService() {
		return *audioService;
	}
	InputService& getInputService() {
		return *inputService;
	}
};

template <>
struct CreateFunctor<System> {
    System *operator()(const std::string& x) const {
        return new System(x);
    }
    System *operator()(const std::string& x, const std::string& y) const {
        return new System(x, y);
    }
};

} // namespace Core
} // namespace Ego
