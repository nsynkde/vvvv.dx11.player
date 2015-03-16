#pragma once
#include <condition_variable>
#include <mutex>
#include <chrono>
#include "HighResClock.h"

class Timer{
public:

	Timer(std::chrono::nanoseconds period);

	void wait_next();
	void reset();
	void set_period(std::chrono::nanoseconds period);
private:
	HighResClock::time_point start_time;
	HighResClock::time_point next_event;
	std::chrono::nanoseconds period;
	std::mutex m;
	std::condition_variable c;
	std::unique_lock<std::mutex> l;
};