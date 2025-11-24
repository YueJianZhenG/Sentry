//
// Created by yy on 2025/7/18.
//

#pragma once

#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"
#else
#include<cstdlib>
#endif
namespace wrap
{
	template<bool FREE = false>
	class string
	{
	public:
		string() : mString(nullptr), mSize(0)
		{
		}

		string(const char* str, size_t size) : mString(str), mSize(size)
		{
		}

	public:
		~string()
		{
			if (FREE)
			{
#ifdef __ENABLE_MI_MALLOC__
				mi_free((void*)this->mString);
#else
				free((void*)this->mString);
#endif
			}
		}

	public:
		inline void assign(const char* str, size_t size)
		{
			if (FREE)
			{
#ifdef __ENABLE_MI_MALLOC__
				mi_free((void*)this->mString);
#else
				free((void*)this->mString);
#endif
				this->mSize = 0;
				this->mString = nullptr;
			}
			this->mSize = size;
			this->mString = str;
		}

	public:
		inline size_t size() const { return this->mSize; }
		inline const char* c_str() const { return this->mString; }
	private:
		size_t mSize;
		const char* mString;
	};
}