#include "stdafx.h"
#include "WorkerThread.h"
#include <sstream>

namespace
{
    const long long g_Frequency = []() -> long long 
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        return frequency.QuadPart;
    }();
}

HighResClock::time_point HighResClock::now()
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return time_point(duration(count.QuadPart * static_cast<rep>(period::den) / g_Frequency));
}

WorkerThread::WorkerThread()
	:isRunning(true)
	,thread(&WorkerThread::thread_loop,this)
	,one_sec(HighResClock::now())
	,num_works(0)
{

}

void WorkerThread::thread_loop(){
	std::unique_lock<std::mutex> lock(mutex);
	
	std::stringstream str;
	str << "clock precision: " << std::chrono::high_resolution_clock::period::den;
	OutputDebugStringA(str.str().c_str());

	while(is_running()){
		//OutputDebugStringA("Waiting");
		condition.wait(lock);
		//OutputDebugStringA("Notified");

		while(is_running() && !works.empty()){
			//OutputDebugStringA("Working");
			std::function<void()> work = works.back();
			works.pop();
			lock.unlock();
			auto start_time = HighResClock::now();
			work();
			auto end_time = HighResClock::now();
			auto last_work_time = end_time - start_time;
			++num_works;
			work_acc += last_work_time;
			if(work_max_duration<last_work_time) work_max_duration = last_work_time;
			if(std::chrono::duration_cast<std::chrono::seconds>(end_time - one_sec) >= std::chrono::seconds(1)){
				one_sec = end_time;
				work_avg_duration = work_acc / num_works;
				work_acc = std::chrono::microseconds::zero();
				work_max_duration = std::chrono::microseconds::zero();
				num_works = 0;
			}
			lock.lock();
		}
	}

	OutputDebugStringA("end thread");
}


bool WorkerThread::is_running(){
	return isRunning;
}

void WorkerThread::add_work(std::function<void()> work){
	std::unique_lock<std::mutex> lock(mutex);
	works.push(work);
	condition.notify_all();
}

HighResClock::duration WorkerThread::get_work_avg_duration(){
	return work_avg_duration;
}

HighResClock::duration WorkerThread::get_work_max_duration(){
	return work_max_duration;
}

void WorkerThread::stop(){
	OutputDebugStringA("locking");
	std::unique_lock<std::mutex> lock(mutex);
	OutputDebugStringA("running false");
	isRunning = false;
	OutputDebugStringA("notify all");
	condition.notify_all();
}

void WorkerThread::join(){
	thread.join();
}