#include "stdafx.h"
#include "Pool.h"



Pool & Pool::GetInstance(){
	static Pool * pool = new Pool;
	return *pool;
}

void ReleaseContext(Context * context){
	Pool::GetInstance().contexts.emplace_back(context,&ReleaseContext);
}

std::shared_ptr<Context> Pool::AquireContext(const Format & format){
	std::unique_lock<std::mutex> lock(mutex);
	auto context_it = contexts.begin();
	while(context_it != contexts.end() && (*context_it)->GetFormat()!=format){
		++context_it;
	}
	if(context_it == contexts.end()){
		// Create an extra context since will surely need it at some point
		contexts.push_back(std::shared_ptr<Context>(new Context(format),&ReleaseContext));
		return std::shared_ptr<Context>(new Context(format),&ReleaseContext);
	}else{
		auto context = *context_it;
		contexts.erase(context_it);
		return context;
	}
}

