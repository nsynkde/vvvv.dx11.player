
#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "Context.h"
#include "ImageFormat.h"

class Pool
{
public:

	static Pool & GetInstance();

	std::shared_ptr<Context> AquireContext(const std::string & fileForFormat);
	int Size();
private:
	friend void ReleaseContext(Context * context);
	Pool(void){};
	std::vector<std::shared_ptr<Context>> contexts;
	std::mutex mutex;
};
