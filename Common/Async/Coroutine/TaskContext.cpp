#include"TaskContext.h"
#include<cstring>
#ifndef __OS_WIN__
#include <sys/mman.h>
#endif

#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"
#endif
namespace acs
{
	TaskContext::TaskContext()
	{
		this->id = 0;
#ifdef __ENABLE_SHARE_STACK__
		this->sid = 0;
#endif
		this->timerId = 0;
		this->ctx = nullptr;
		this->cb = nullptr;
		this->status = CorState::Ready;
		std::memset(&this->stack, 0, sizeof(Stack));
	}

	void TaskContext::Invoke()
	{
		this->cb->run();
		this->status = CorState::Finish;
	}

	TaskContext::~TaskContext()
	{
		if (this->stack.p)
		{
#ifdef __ENABLE_MI_MALLOC__
			mi_free(this->stack.p);
#else
			free(this->stack.p);
#endif
			std::memset(&this->stack, 0, sizeof(Stack));
		}
	}
}