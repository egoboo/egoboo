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

#pragma once

#include "egolib/platform.h"

namespace Ego
{
namespace Core
{

/**
 * @brief
 *  A timer service.
 * @author
 *  Michael Heilmann
 */
class TimerService : public Id::NonCopyable
{
protected:
    friend class System;
    TimerService();
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
 * @brief
 *  An event threading service.
 * @author
 *  Michael Heilmann
 */
class EventService : public Id::NonCopyable
{
protected:
    friend class System;
    EventService();
    virtual ~EventService();
};

class System : public Id::NonCopyable
{

private:

    /**
     * @brief
     *  The singleton instance of the system.
     */
    static System *_singleton;

protected:

    /**
     * @brief
     *  Construct this system.
     * @remark
     *  Intentionally protected.
     */
    System(const char *binaryPath, const char *egobooPath);

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

    /**
     * @brief
     *  Get the singleton instance of the system.
     * @return
     *  the singleton instance of the system
     * @throw std::logic_error
     *  if the system is not initialized
     */
    static System& get();

    /**
     * @brief
     *  Initialize the system singleton.
     * @remark
     *  If the system singleton is initialized,
     *  a call to this method is a no-op.
     */
    static void initialize(const char *binaryPath, const char *egobooPath);

    /**
     * @brief
     *  Uninitialize the system singleton.
     * @remark
     *  If the system singleton is not initialized,
     *  a call to this method is a no-op.
     */
    static void uninitialize();

private:
    TimerService *_timerService;
    EventService *_eventService;
public:
    TimerService &getTimerService()
    {
        return *_timerService;
    }
    EventService& getEventService()
    {
        return *_eventService;
    }
};

} // namespace Core
} // namespace Ego
