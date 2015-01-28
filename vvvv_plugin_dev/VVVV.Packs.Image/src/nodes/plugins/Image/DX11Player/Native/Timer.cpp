#include "stdafx.h"
#include "Timer.h"
#include <thread>
#include <Winsock2.h>

Timer::Timer(std::chrono::nanoseconds period)
:start_time(HighResClock::now())
,next_event(start_time+std::chrono::nanoseconds(period))
,period(period)
,l(m)
{
}

void Timer::wait_next(){
	//c.wait_until(l,next_event);
	
	//auto sleep_for = next_event - HighResClock::now();
	//std::this_thread::sleep_for(sleep_for);

	/*timeval t;
	t.tv_sec = 0;
	t.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(next_event - HighResClock::now()).count();
	select(0,NULL,NULL,NULL,&t);*/
	c.wait_until(l,next_event);
	next_event += period;

}


void Timer::set_period(std::chrono::nanoseconds period){
	next_event -= this->period;
	this->period = period;
}