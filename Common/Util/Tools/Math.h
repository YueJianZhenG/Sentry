#pragma once


#include<string>

namespace help
{
	namespace Math
	{
		template<typename T>
		inline const T& Min(const T& t1, const T& t2)
		{
			return t1 < t2 ? t1 : t2;
		}

		template<typename T>
		inline const T& Max(const T& t1, const T& t2)
		{
			return t1 > t2 ? t1 : t2;
		}

		template<typename T>
		inline const T& Clamp(const T& num, const T& min, const T& max)
		{
			if (num >= min && num <= max)
			{
				return num;
			}
			return num < min ? min : max;
		}

		template<typename T>
		inline const T& MinZero(const T& t1, const T& t2)
		{
			return t1 == 0 ? t2 : t2 == 0 ? t1
										  : t1 < t2 ? t1
													: t2;
		}

		template<typename T>
		inline const T& MaxZero(const T& t1, const T& t2)
		{
			return t1 == 0 ? t2 : t2 == 0 ? t1
										  : t1 > t2 ? t1
													: t2;
		}

		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value, bool> ToNumber(const std::string& str, T& value)
		{
			try
			{
				value = (T)std::stoll(str);
				return true;
			}
			catch(const std::exception & e)
			{
				return false;
			}
			catch(...)
			{
				return false;
			}
		}

		template<typename T>
		inline std::enable_if_t<std::is_floating_point<T>::value, bool> ToNumber(const std::string& str, T& value)
		{
			try
			{
				value = (T)std::stod(str);
				return true;
			}
			catch(const std::exception & e)
			{
				return false;
			}
			catch(...)
			{
				return false;
			}
		}
	}
}

// namespace MathHelper