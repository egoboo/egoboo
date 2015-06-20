#pragma once

#include "egolib/typedef.h"

namespace Ego {
namespace Time {

/**
 * @brief
 *  The interface of a stopwatch.
 * @author
 *  Michael Heilmann
 */
struct Stopwatch {
private:
    /**
     * @brief
     * If the stopwatch is running.
     */
    bool _running;
    /**
     * @brief
     *  The point in time at which the stopwatch was started.
     */
    std::chrono::high_resolution_clock::time_point _begin;
    /**
     * @brief
     *  The point in time at which the stopwatch was stopped.
     */
    std::chrono::high_resolution_clock::time_point _end;
 public:
    /**
     * @brief
     *  Construct this stopwatch.
     * @post
     *  - <tt>running = false</tt>
     *  - <tt>end = begin = now</tt>
     */
    Stopwatch()
        : _running(false),
          _begin(std::chrono::high_resolution_clock::now()),
          _end(std::chrono::high_resolution_clock::now()) {
    }
    /**
     * @brief
     *  Get if this stopwatch is running.
     * @return
     *  @a true if this stopwatch is running,
     *  @a false otherwise
     * @post
     *  - <tt>result = running</tt>
     */
    bool isRunning() const {
        return _running;
    }
    /**
     * @brief
     *  Get the elapsed time duration.
     * @return
     *  the elapsed time duration
     * @remark
     *  If the stopwatch is running, the elapsed time <tt>now - begin</tt> is returned.
     *  If the stopwatch is stopped, the elasped time <tt>end - begin</tt> is returned.
     * @post
     *  <tt>end = now</tt> if <tt>running = true</tt>
     *  <tt>result = end@new - begin</tt>
     */
    double elapsed() {
        if (_running) {
            _end = std::chrono::high_resolution_clock::now();
        }
        return std::chrono::duration_cast<std::chrono::duration<double>>(_end - _begin).count();
    }
    /**
     * @brief
     *  Start this stopwatch.
     * @remark
     *  No effect is this stopwatch is started.
     * @post
     *  If <tt>running = false</tt> then
     *  - <tt>running = true</tt>
     *  - <tt>end = now</tt>
     * otherwise
     *  - do nothing
     */
    void start() {
        if (!_running) {
            _begin = std::chrono::high_resolution_clock::now();
            _running = true;
        }
    }
    /**
     * @brief
     *  Stop this stopwatch.
     * @remark
     *  No effect if this stopwatch is stopped.
     * @post
     *  If <tt>running = true</tt> then
     *  - <tt>running = false</tt>
     *  - <tt>end = now</tt>
     *  otherwise
     *  - do nothing
     */
    void stop() {
        if (_running) {
            _end = std::chrono::high_resolution_clock::now();
            _running = false;
        }
    }
    /**
     * @brief
     *  Reset this stopwatch.
     * @post
     *  <tt>begin = end = now</tt>
     */
     void reset() {
        _begin = std::chrono::high_resolution_clock::now();
        _end = _begin;
     }
};

} // Time
} // Ego