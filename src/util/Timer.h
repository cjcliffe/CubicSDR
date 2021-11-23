// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#ifndef TIMER_H
#define TIMER_H

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/// Timer Class, high resolution timer
/**
 * Class provides high resolution timing and useful time control functions
 */

class Timer {
private:

    unsigned long time_elapsed;
    unsigned long system_milliseconds;
    unsigned long start_time;
    unsigned long end_time;
    unsigned long last_update;
    unsigned long num_updates;
    unsigned long paused_time;
    unsigned long offset;

#ifndef _WIN32
    struct timeval time_val;
    struct timezone time_zone;
#else
    LARGE_INTEGER win_frequency;
#endif

    bool paused_state;
    bool lock_state;
    float lock_rate;

public:

    /// Constructor
    Timer();

    /// Start the timer
    /**
     *  Resets the timer to 0 and begins timing
     */
    void start();

    /// Stop the timer
    /**
     *  Stops the timer and records the end time
     */
    void stop();

    /// Locks the timer to a specified framerate (for recording / benchmarking purposes typically)
    /**
     *  Locks the timer to a specified framerate (for recording / benchmarking purposes typically)
     */
    void lockFramerate(float f_rate);

    /// Unlock any framerate lock that's been applied
    /**
     *  Unlock any framerate lock that's been applied
     */
    void unlock();

    /// Check locked state
    /**
     *  Check locked state
     */
    bool locked() const;

    /// Reset the timer counter
    /**
     *	Resetting the timer will reset the current time to 0
     */
    void reset();

    /// Timer update
    /**
     *	Calling the update command will bring the timer value up to date, this is meant
     *	to be called at the begining of the frame to establish the time index which is being drawn.
     */
    void update();

    /// Get the total time elapsed since the timer start, not counting paused time
    /**
     *	Returns the total time elapsed in since the timer start() to the last update() but
     *	does not count the time elapsed while the timer is paused().
     *  \return Total time elapsed since the timer start() to the last update() excluding time paused() in milliseconds
     */
    unsigned long getMilliseconds() const;

    /// Alias of getMilliseconds() which returns time in seconds
    /**
     *  \return Total time elapsed since the timer start() to the last update() excluding time paused() in seconds
     */
    double getSeconds() const;

    /// Get the total time elapsed since the timer start
    /**
     *	Returns the total time elapsed in since the timer start() to the last update()
     *  this includes any time accumulated during updates while paused()
     *  \return Total time elapsed since the timer start() to the last update() including time paused() in milliseconds
     */
    unsigned long totalMilliseconds() const;
    /// Alias of totalMilliseconds() which returns time in seconds
    /**
     *  \return Total time elapsed since the timer start() to the last update() including time paused() in seconds
     */
    double totalSeconds() const;

    /// Set the amount of time elapsed
    /**
     *	Force the timer duration to a specific value, useful for rolling forward or back in a system
     *	based upon the timer.
     *  \param milliseconds_in Time to set timer to in milliseconds
     */
    void setMilliseconds(unsigned long milliseconds_in);
    /// alias of setMilliseconds() which accepts time in seconds
    /**
     *  \param seconds_in Time to set timer to in seconds
     */
    void setSeconds(double seconds_in);

    /// Get the amount of times the update() command has been called
    /**
     * 	By using the number of times the update() command has been called you can easily determine
     *  an average frame rate. Also useful for merely determining how many frames have been drawn.
     *  \return Number of times update() has been called
     */
    unsigned long getNumUpdates() const;

    /// Get the timer duration during the last update
    /**
     *	Useful for determining the amount of time which elapsed during the last update
     *	can be used to accurately control values with a per-second rate or determine the current frame rate.
     *  \return Duration of time between the last two calls to update() in milliseconds
     */
    unsigned long lastUpdateMilliseconds() const;
    /// Alias of lastUpdateMilliseconds() which returns time in seconds
    /**
     *  \return Duration of time between the last two calls to update() in seconds
     */
    double lastUpdateSeconds() const;

    /// Set the timer pause state
    /**
     *  Pause the timer, allowing for continued update() calls without an increment in timing but
     *	maintaining the update and total time count, useful for pausing a scene but allowing frame
     *	timing to resume.
     *  \param pause_in Value to set the current pause state to
     */
    void paused(bool pause_in);

    /// Check if the timer is currently in a paused state
    /**
     *  \return Current pause state, true if paused, false otherwise
     */
    bool paused() const;
    

    
    void timerTestFunc();
};

#endif

