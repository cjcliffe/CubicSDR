/*
    This file is part of CubicVR.

    CubicVR is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    CubicVR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with CubicVR; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    
    Source (c) 2003 by Charles J. Cliffe unless otherwise specified
    To contact me, e-mail or MSN to cj@cubicproductions.com or by ICQ#7188044
    http://www.cubicproductions.com
*/

#include "Timer.h"

#ifdef _WIN32
	#include <mmsystem.h>
#endif

Timer::Timer(void) : time_elapsed(0), system_milliseconds(0), start_time(0), end_time(0), last_update(0), paused_time(0), offset(0), paused_state(false)
{
}


void Timer::start(void) 
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


void Timer::stop(void) 
{
	end_time = system_milliseconds;
}


void Timer::reset(void)
{
	start();
}


void Timer::lockFramerate(float f_rate)
{
	lock_rate = 1.0/f_rate;
	lock_state = true;
}


void Timer::unlock()
{
	unsigned long msec_tmp = system_milliseconds;
	
	lock_state = false;

	update();
	
	last_update = system_milliseconds-lock_rate;
	
	offset += msec_tmp-system_milliseconds;
	
	lock_rate = 0;
}

bool Timer::locked()
{
	return lock_state;
}

void Timer::update(void) 
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
		system_milliseconds = timeGetTime ();
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


unsigned long Timer::getMilliseconds(void) 
{
	return time_elapsed;
}



double Timer::getSeconds(void) 
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


double Timer::lastUpdateSeconds(void)
{
	return ((double)lastUpdateMilliseconds())/1000.0;
}


unsigned long Timer::lastUpdateMilliseconds(void)
{
	return system_milliseconds-last_update;
}

unsigned long Timer::totalMilliseconds()
{
	return system_milliseconds-start_time;
}


double Timer::totalSeconds(void)
{
	return totalMilliseconds()/1000.0;
}


unsigned long Timer::getNumUpdates(void)
{
	return num_updates;
}


void Timer::paused(bool pause_in)
{
	paused_state = pause_in;
}

bool Timer::paused()
{
	return paused_state;
}
