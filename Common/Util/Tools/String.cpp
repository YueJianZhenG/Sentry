#include"String.h"
#include<regex>
#include<sstream>
#include <codecvt>

#ifdef __OS_WIN__

#include <Windows.h>

#endif
#include "Math.h"
#include "Random.h"

namespace help
{
	std::string Str::EmptyStr;

	const std::string& Str::Empty()
	{
		return EmptyStr;
	}

	const std::string& Str::Tolower(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

	const std::string& Str::Toupper(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	size_t Str::Hash(const std::string& str)
	{
		size_t hash = 14695981039346656037ULL;
		for (char c : str)
		{
			hash ^= static_cast<size_t>(c);
			hash *= 1099511628211ULL;
		}
		return hash;
	}

	bool Str::IsRegex(const std::string& pattern)
	{
		try
		{
			std::regex re(pattern, std::regex_constants::ECMAScript);
			return true;
		}
		catch (const std::regex_error& e)
		{
			return false;
		}
	}


	size_t Str::Split(const std::string& targetString, char cc, std::vector<std::string>& ret)
	{
		std::string item;
		std::stringstream ss(targetString);
		while (std::getline(ss, item, cc))
		{
			ret.emplace_back(item);
		}
		return ret.size();
	}

	size_t Str::Split(const std::string& targetString, char cc, std::vector<int>& ret)
	{
		std::string item;
		std::stringstream ss(targetString);
		while (std::getline(ss, item, cc))
		{
			int value = 0;
			if (help::Math::ToNumber(item, value))
			{
				ret.emplace_back(value);
			}
		}
		return ret.size();
	}


	size_t Str::Split(const std::string& target, char cc, std::string& str1, std::string& str2)
	{
		size_t pos = target.find(cc);
		if (pos == std::string::npos)
		{
			return -1;
		}
		str2.clear();
		str1.assign(target.c_str(), pos);
		if (target.size() > pos + 1)
		{
			str2.assign(target.c_str() + pos + 1);
		}
		return 0;
	}

	void Str::Replace(std::string& outstring, const std::string& str1, const std::string& str2)
	{
		int index = 0;
		size_t pos = outstring.find(str1);
		while (pos != std::string::npos && index < 100)
		{
			index++;
			outstring.replace(pos, str1.length(), str2);
			pos = outstring.find(str1);
		}
	}

	extern void Str::ClearBlank(std::string& input)
	{
		input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
	}

	bool Str::GetFileName(const std::string& path, std::string& name)
	{
		size_t pos = path.find_last_of('/');
		if (pos != std::string::npos)
		{
			name = path.substr(pos + 1, path.size());
			return true;
		}
		pos = path.find_last_of('\\');
		if (pos != std::string::npos)
		{
			name = path.substr(pos + 1, path.size());
			return true;
		}

		return false;
	}

	std::string Str::FormatJson(const std::string& json)
	{
		std::string format;
		auto getLevelStr = [](int level, std::string& str)
		{
			for (int i = 0; i < level; i++)
			{
				str += "    "; //这里可以\t换成你所需要缩进的空格数
			}
		};

		int level = 0;
		for (size_t index = 0; index < json.size(); index++)
		{
			char c = json[index];

			if (level > 0 && '\n' == json[json.size() - 1])
			{
				getLevelStr(level, format);
			}

			switch (c)
			{
			case '{':
			case '[':
				format = format + c + "\n";
				level++;
				getLevelStr(level, format);
				break;
			case ',':
				format = format + c + "\n";
				getLevelStr(level, format);
				break;
			case '}':
			case ']':
				format += "\n";
				level--;
				getLevelStr(level, format);
				format += c;
				break;
			default:
				format += c;
				break;
			}
		}
		return format;
	}

	std::string Str::RandomString(int size)
	{
		std::string result;
		result.resize(size);
		static const std::string STR_BUFFER("123456789qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM");
		for (size_t index = 0; index < size; index++)
		{
			int max = (int)STR_BUFFER.size();
			int num = help::Rand::Random<int>(0, max - 1);
			result[index] = (char)STR_BUFFER[num];
		}
		return result;
	}
}

namespace help
{
	bool Str::SplitAddr(const std::string& address, std::string& net, std::string& ip, unsigned short& port)
	{
		std::smatch match;
		std::regex pattern(R"((\w+)://(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+))");
		if (!std::regex_search(address, match, pattern))
		{
			return false;
		}
		net = match[1].str();
		ip = match[2].str();
		port = (unsigned short)std::stoi(match[3].str());
		return true;
	}

	bool Str::SplitAddr(const std::string& address, std::string& ip, unsigned short& port)
	{
		std::smatch match;
		std::regex pattern(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+))");
		if (!std::regex_search(address, match, pattern))
		{
			return false;
		}
		ip = match[1].str();
		port = (unsigned short)std::stoi(match[2].str());
		return true;
	}

	bool Str::IsIpAddress(const std::string& str)
	{
		std::regex pattern(R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$)");
		return std::regex_match(str, pattern);
	}

	bool Str::IsRpcAddr(const std::string& address)
	{
		//std::regex pattern(R"(^\d{1,3}(\.\d{1,3}){3}:\d{1,5}$)");
		std::regex pattern(R"(tcp://[\w\-_]+(\.[\w\-_]+)+([\w\-\.,@?^=%&:/~\+#]*[\w\-\@?^=%&/~\+#])?)");
		return std::regex_match(address, pattern);
	}

	bool Str::IsHttpAddr(const std::string& address)
	{
		std::regex pattern(R"((http|https)://[\w\-_]+(\.[\w\-_]+)+([\w\-\.,@?^=%&:/~\+#]*[\w\-\@?^=%&/~\+#])?)");
		return std::regex_match(address, pattern);
	}

	bool Str::IsPhoneNumber(const std::string& phoneNumber)
	{
		if (phoneNumber.empty())
		{
			return false;
		}
		std::regex pattern("^1[3456789]\\d{9}$");
		return std::regex_match(phoneNumber, pattern);
	}
}

namespace help
{
	size_t utf8::Length(const std::string& str)
	{
		try
		{
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8_utf32_converter;
			std::u32string utf32_str = utf8_utf32_converter.from_bytes(str);
			return utf32_str.size();
		}
		catch (const std::range_error&)
		{
			return str.size();
		}
	}

	bool utf8::IsByte(unsigned char c)
	{
		return (c & 0xF0) == 0xE0 || (c & 0xE0) == 0xC0 || (c & 0x80) == 0x00;
	}

	bool utf8::IsValid(const std::string& str)
	{
		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(str.data());
		size_t length = str.size();
		size_t i = 0;

		while (i < length)
		{
			uint8_t byte = bytes[i];

			// 1 字节字符 (0xxxxxxx)
			if ((byte & 0x80) == 0x00)
			{
				i += 1;
				continue;
			}

			// 2 字节字符 (110xxxxx 10xxxxxx)
			if ((byte & 0xE0) == 0xC0)
			{
				if (i + 1 >= length) return false;
				if ((bytes[i + 1] & 0xC0) != 0x80) return false;
				if (byte == 0xC0 || byte == 0xC1) return false; // 过度编码
				i += 2;
				continue;
			}

			// 3 字节字符 (1110xxxx 10xxxxxx 10xxxxxx)
			if ((byte & 0xF0) == 0xE0)
			{
				if (i + 2 >= length) return false;
				if ((bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) return false;

				// 禁止过度编码 (U+0080–U+07FF)
				if (byte == 0xE0 && (bytes[i + 1] & 0xE0) == 0x80) return false;

				// 禁止代理区 (U+D800–U+DFFF)
				if (byte == 0xED && (bytes[i + 1] & 0xE0) == 0xA0) return false;

				i += 3;
				continue;
			}

			// 4 字节字符 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
			if ((byte & 0xF8) == 0xF0)
			{
				if (i + 3 >= length) return false;
				if ((bytes[i + 1] & 0xC0) != 0x80 ||
					(bytes[i + 2] & 0xC0) != 0x80 ||
					(bytes[i + 3] & 0xC0) != 0x80)
					return false;

				// 检查 Unicode 范围是否超出 U+10FFFF
				if (byte > 0xF4 || (byte == 0xF4 && (bytes[i + 1] & 0xF0) > 0x80))
					return false;

				i += 4;
				continue;
			}

			// 其他非法模式
			return false;
		}

		return true;
	}

	std::string utf8::Sub(const std::string& str, size_t n)
	{
		return utf8::Sub(str, 0, n);
	}

	extern std::string utf8::Sub(const std::string& str, size_t s, size_t n)
	{
		if (str.empty() || s >= utf8::Length(str) || n == 0)
		{
			return "";
		}

		try
		{
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8_utf32_converter;
			std::u32string utf32_str = utf8_utf32_converter.from_bytes(str);
			size_t total_chars = utf32_str.size();

			size_t start = std::min(s, total_chars - 1);
			size_t count = std::min(n, total_chars - start);

			std::u32string utf32_sub = utf32_str.substr(start, count);

			return utf8_utf32_converter.to_bytes(utf32_sub);
		}
		catch (const std::range_error&)
		{
			// 非法 UTF-8 编码时，降级处理（返回空或原字符串片段，根据需求调整）
			return str.substr(s, n); // 仅作降级，可能拆分多字节字符
		}
	}
}

namespace help
{
#ifdef __OS_WIN__
	namespace text
	{
		// GB2312(CP_ACP) 转 UTF-8
		std::string GB2312ToUtf8(const std::string& text)
		{

			if (help::utf8::IsValid(text))
			{
				return text;
			}
			int ansi_len = static_cast<int>(text.size());
			// 计算所需宽字符数（不含终止符）
			int wchar_len = MultiByteToWideChar(CP_ACP, 0, text.c_str(), ansi_len, nullptr, 0);
			if (wchar_len <= 0)
			{
				return ""; // 转换失败
			}
			// 分配宽字符缓冲区（+1 预留终止符）
			std::unique_ptr<wchar_t[]> wstr = std::make_unique<wchar_t[]>(wchar_len + 1);
			// 执行转换（最后一个参数传入缓冲区总大小）
			int result = MultiByteToWideChar(CP_ACP, 0, text.c_str(), ansi_len, wstr.get(), wchar_len + 1);
			if (result <= 0)
			{
				return ""; // 转换失败
			}

			// 第二步：宽字符转 UTF-8
			// 计算所需 UTF-8 字节数（包含终止符）
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.get(), -1, nullptr, 0, nullptr, nullptr);
			if (utf8_len <= 0)
			{
				return ""; // 转换失败
			}
			// 分配 UTF-8 缓冲区（无需 +1，因为 utf8_len 已包含终止符）
			std::unique_ptr<char[]> utf8_str = std::make_unique<char[]>(utf8_len);
			// 执行转换
			int written = WideCharToMultiByte(CP_UTF8, 0, wstr.get(), -1, utf8_str.get(), utf8_len, nullptr,
			                                  nullptr);
			if (written <= 0)
			{
				return ""; // 转换失败
			}

			// 返回不含终止符的字符串（utf8_len 包含 \0，因此取 utf8_len - 1）
			return std::string(utf8_str.get(), utf8_len - 1);

		}

		// UTF-8 转 GB2312(CP_ACP)
		std::string Utf8ToGB2312(const std::string& text)
		{
			if (text.empty()) return "";
			if (!help::utf8::IsValid(text))
			{
				return text;
			}
			int wchar_len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
			if (wchar_len <= 0)
			{
				return ""; // 转换失败（如无效 UTF-8 字符）
			}

			// 分配宽字符缓冲区（大小为计算出的长度，已包含终止符）
			std::unique_ptr<wchar_t[]> wstr = std::make_unique<wchar_t[]>(wchar_len);

			// 执行转换
			int result = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wstr.get(), wchar_len);
			if (result <= 0)
			{
				return ""; // 转换失败
			}

			// 第二步：宽字符转 ANSI（系统默认编码 CP_ACP）
			// 计算所需 ANSI 字节数（包含终止符 \0）
			int ansi_len = WideCharToMultiByte(CP_ACP, 0, wstr.get(), -1, nullptr, 0, nullptr, nullptr);
			if (ansi_len <= 0)
			{
				return ""; // 转换失败（如宽字符无法映射到 ANSI）
			}

			// 分配 ANSI 缓冲区（大小为计算出的长度，已包含终止符）
			std::unique_ptr<char[]> ansi_str = std::make_unique<char[]>(ansi_len);

			// 执行转换
			int written = WideCharToMultiByte(CP_ACP, 0, wstr.get(), -1, ansi_str.get(), ansi_len, nullptr,
			                                  nullptr);
			if (written <= 0)
			{
				return ""; // 转换失败
			}

			// 返回不含终止符的字符串（ansi_len 包含 \0，因此取 ansi_len - 1）
			return std::string(ansi_str.get(), ansi_len - 1);
		}
	} // namespace text

#endif
}
