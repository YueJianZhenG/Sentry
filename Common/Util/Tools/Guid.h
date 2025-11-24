#pragma once
#include<atomic>
#include<string>

namespace help
{
    namespace  ID
    {
    	extern long long Gen();
        extern long long Make();
		extern long long Make(int count);
    };
}// namespace Guid