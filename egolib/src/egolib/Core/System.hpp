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

/**
 * @brief A timer service.
 */
class TimerService : public Id::NonCopyable {
protected:
    friend class System;
    friend std::unique_ptr<TimerService> std::make_unique<TimerService>();
    friend std::unique_ptr<TimerService>::deleter_type;
    explicit TimerService();
    virtual ~TimerService();
public:
    /**
     * @brief
     *  Returns the number of milliseconds since the initialization of the timer service.
     * @return
     *  the number of milliseconds since the initialization of the timer service.
     * @remark 
     *  This value wraps around if the program runs for more than 49.7 days. 
     */
    uint32_t getTicks();
};

/**
 * @brief An event threading service.
 */
class EventService : public Id::NonCopyable {
protected:
    friend class System;
    friend std::unique_ptr<EventService> std::make_unique<EventService>();
    friend std::unique_ptr<EventService>::deleter_type;
    explicit EventService();
    virtual ~EventService();
};

/**
 * @brief A video service.
 */
class VideoService : public Id::NonCopyable {
protected:
	friend class System;
    friend std::unique_ptr<VideoService> std::make_unique<VideoService>();
    friend std::unique_ptr<VideoService>::deleter_type;
	explicit VideoService();
	virtual ~VideoService();
};

/**
 * @brief An audio service.
 */
class AudioService : public Id::NonCopyable {
protected:
	friend class System;
    friend std::unique_ptr<AudioService> std::make_unique<AudioService>();
    friend std::unique_ptr<AudioService>::deleter_type;
	explicit AudioService();
	virtual ~AudioService();
};

/**
 * @brief An input service.
 */
class InputService : public Id::NonCopyable {
protected:
	friend class System;
    friend std::unique_ptr<InputService> std::make_unique<InputService>();
    friend std::unique_ptr<InputService>::deleter_type;
    explicit InputService();
	virtual ~InputService();
};



class System : public Singleton<System> {
protected:
    friend struct CreateFunctor<System>;
    friend struct DestroyFunctor<System>;
    /**
     * @brief
     *  Construct this system.
     * @remark
     *  Intentionally protected.
     */
    System(const std::string& binaryPath, const std::string& egobooPath);
    System(const std::string& binaryPath);

    /**
     * @brief
     *  Destruct this system.
     * @remark
     *  Intentionally protected.
     */
    virtual ~System();

public:

    /**
     * @brief
     *  The version of the Egoboo Engine.
     */
    static const std::string VERSION;

private:
    std::unique_ptr<TimerService> timerService;
    std::unique_ptr<EventService> eventService;
    std::unique_ptr<VideoService> videoService;
    std::unique_ptr<AudioService> audioService;
    std::unique_ptr<InputService> inputService;
public:
    TimerService &getTimerService() {
        return *timerService;
    }
    EventService& getEventService() {
        return *eventService;
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
