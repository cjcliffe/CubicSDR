// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <iostream>
#include "Timer.h"

Timer::Timer() : time_elapsed(0), system_milliseconds(0), start_time(0), end_time(0), last_update(0), num_updates(0), paused_time(0), offset(0), paused_state(false), lock_state(false), lock_rate(0)
{
#ifdef _WIN32
    // According to Microsoft, QueryPerformanceXXX API is perfectly
    //fine for Windows 7+ systems, and use the highest appropriate counter.
    //this only need to be done once.
    ::QueryPerformanceFrequency(&win_frequency);
#endif
}


void Timer::start()
{
    update();
    num_updates = 0;
    start_time = system_milliseconds;
    last_update = start_time;
    paused_state = false;
    lock_state = false;
    lock_rate = 0;
    paused_time = 0;
    offset = 0;
}


void Timer::stop()
{
    end_time = system_milliseconds;
}


void Timer::reset()
{
    start();
}


void Timer::lockFramerate(float f_rate)
{
    lock_rate = 1.0f/f_rate;
    lock_state = true;
}


void Timer::unlock()
{
    unsigned long msec_tmp = system_milliseconds;
    
    lock_state = false;

    update();
    
    last_update = system_milliseconds-(unsigned long)lock_rate;
    
    offset += msec_tmp-system_milliseconds;
    
    lock_rate = 0;
}

bool Timer::locked() const
{
    return lock_state;
}

void Timer::update()
{
    num_updates++;
    last_update = system_milliseconds;
    
    
    if (lock_state)
    {
        system_milliseconds += (unsigned long)(lock_rate*1000.0);
    }
    else
    {
#ifdef _WIN32

        //Use QuaryPerformanceCounter, imune to problems sometimes
        //multimedia timers have.
        LARGE_INTEGER win_current_count;
        ::QueryPerformanceCounter(&win_current_count);

        system_milliseconds = (unsigned long)(win_current_count.QuadPart * 1000.0 / win_frequency.QuadPart);

#else
        gettimeofday(&time_val,&time_zone);

        system_milliseconds = (unsigned long)time_val.tv_usec;
        system_milliseconds /= 1000;
        system_milliseconds += (unsigned long)(time_val.tv_sec*1000);
#endif
    }


    if (paused_state) paused_time += system_milliseconds-last_update;

    time_elapsed = system_milliseconds-start_time-paused_time+offset;
}


unsigned long Timer::getMilliseconds() const
{
    return time_elapsed;
}



double Timer::getSeconds() const
{
    return ((double)getMilliseconds())/1000.0;
}


void Timer::setMilliseconds(unsigned long milliseconds_in) 
{
    offset -= (system_milliseconds-start_time-paused_time+offset)-milliseconds_in;
}



void Timer::setSeconds(double seconds_in) 
{
    setMilliseconds((long)(seconds_in*1000.0));
}


double Timer::lastUpdateSeconds() const
{
    return ((double)lastUpdateMilliseconds())/1000.0;
}


unsigned long Timer::lastUpdateMilliseconds() const
{
    return system_milliseconds-last_update;
}

unsigned long Timer::totalMilliseconds() const
{
    return system_milliseconds-start_time;
}


double Timer::totalSeconds() const
{
    return totalMilliseconds()/1000.0;
}


unsigned long Timer::getNumUpdates() const
{
    return num_updates;
}


void Timer::paused(bool pause_in)
{
    paused_state = pause_in;
}

bool Timer::paused() const
{
    return paused_state;
}

void Timer::timerTestFunc() {
    update();
    if (getNumUpdates() % 120 == 0) {
        std::cout << getNumUpdates() << "," << getSeconds() << " Rate: " << ((double)getNumUpdates()/getSeconds()) << "/sec" << std::endl;
    }
    if (getNumUpdates() >= 600) {
        reset();
    }
}

