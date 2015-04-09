
#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "Context.h"

class Pool
{
public:

	static Pool & GetInstance();

	std::shared_ptr<Context> AquireContext(const Format & format);
	int Size();
private:
	friend void ReleaseContext(Context * context);
	Pool(void){};
	std::vector<std::shared_ptr<Context>> contexts;
	std::mutex mutex;
};
