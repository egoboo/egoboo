#pragma once

/**
 * @brief
 *  Encapsulates the state of a 2D animation loop.
 * @author
 *  Michael Heilmann
 */
struct AnimationLoop
{
    /// The index of the first frame.
    int _start;
    /// The number of frames.
    int _count;
    /// A constant to be added to _current at each update.
    int _add;
    /// The offset of the current from from the first frame.
    int _offset;
    AnimationLoop() :
        _start(0),
        _count(0),
        _add(0),
        _offset(0)
    {}
    void reset()
    {
        _start = 0;
        _count = 0;
        _add = 0;
        _offset = 0;
    }
    /// @brief Get the update count of this animation.
    /// @remark The update count is the number of updates required to reach the frame before
    /// the update which resets the loop to @a 0 from the first frame. It is <tt>ceil(count/
    /// add)-1</tt> if <tt>add != 0</tt> and is @a 0 otherwise.
    /// </br>
    //// <b>Example</b>: For <tt>count = 10</tt> and <tt>add = 3</tt> we start at the initial
    //// frame and increment <tt>0, 3, 6, 9</tt> hence 3 updates are required. As expected, the
    /// update count is <tt>ceil(10/3)-1 = ceil(3.333....) - 1 = 4 - 1 = 3</tt>.
    int getUpdateCount()
    {
        if (!_add)
        {
            return 0;
        }
        else
        {
            return std::ceil((float)(_count) / (float)(_add)) - 1;
        }
    }
};