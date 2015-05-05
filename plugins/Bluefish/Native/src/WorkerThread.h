#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

struct HighResClock
{
    typedef long long                               rep;
    typedef std::nano                               period;
    typedef std::chrono::duration<rep, period>      duration;
    typedef std::chrono::time_point<HighResClock>   time_point;
    static const bool is_steady = true;

    static time_point now();
};

class WorkerThread{
	std::mutex mutex;
	std::condition_variable condition;
	std::queue<std::function<void()>> works;
	bool isRunning;
	std::thread thread;
	HighResClock::time_point one_sec;
	HighResClock::duration work_acc;
	HighResClock::duration work_avg_duration;
	HighResClock::duration work_max_duration;
	uint64_t num_works;
	size_t works_max_limit;
public:
	WorkerThread();
	void thread_loop();
	bool is_running();
	void add_work(std::function<void()>);
	void set_works_max_limit(size_t works_max_limit);
	HighResClock::duration get_work_avg_duration();
	HighResClock::duration get_work_max_duration();
	void stop();
	void join();
	std::mutex & get_mutex();
};