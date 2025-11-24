//
// Created by yjz on 2022/5/18.
//

#ifndef APP_IPROTO_H
#define APP_IPROTO_H
#include<memory>
#include<ostream>
#include<istream>
#include<string>
#include<cstring>
#include<cstdint>
#include<vector>
#include <utility>
struct lua_State;

namespace tcp
{
	namespace read
	{
		constexpr int done = 0;
		constexpr int line = -1; //读一行
		constexpr int some = -2; //读一些
		constexpr int error = -3;    //读取错误
		constexpr int decode_error = -4; //解析错误
		constexpr int pause = -5; //暂停
		constexpr int big_long = -6; //包体过长
		constexpr int all = -7;
		constexpr int content_length = -8;
	}


	namespace Decode
	{
		constexpr int None = 0;
		constexpr int ProtoHead = 1;
		constexpr int MessageHead = 2;
		constexpr int MessageBody = 3;
		constexpr int Done = 4;
	}

	class IProto
	{
	public:
		IProto();
		virtual ~IProto() = default;
	public:
		IProto(IProto&&) = delete;
		IProto(const IProto&) = delete;
		IProto(const IProto&&) = delete;
		IProto& operator=(const IProto&) = delete;
#ifdef __DEBUG__
		long long GetCostTime() const;
#endif
	public:
		virtual void Clear() = 0;
		virtual std::string ToString()
		{
			static std::string empty;
			return empty;
		}
		virtual int OnSendMessage(std::ostream& os) = 0; //返回剩余要发送的字节数
		virtual int OnRecvMessage(std::istream& os, size_t size)
		{
			return 0;
		};
	private:
#ifdef __DEBUG__
		long long mStartTime;
#endif
	};

	namespace Data
	{
		template<typename T>
		union Value
		{
		public:
			T Data;
			char Buffer[sizeof(T)] = { 0 };
		};

		// 判断是否是小端
		inline bool IsLittleEndian() {
			uint16_t value = 0x0001; // 2 字节的值 0x0001
			return (*reinterpret_cast<uint8_t*>(&value) == 0x01); // 检查最低字节是否为 0x01
		}

		template<typename T>
		inline void Read(std::istream& os, T& value)
		{
			char byteArr[sizeof(T)] = { 0 };
			os.readsome(byteArr, sizeof(T));
			memcpy(&value, byteArr, sizeof(T));
		}

		template<typename T>
		inline void Write(std::ostream& is, const T&value)
		{
			char byteArr[sizeof(T)] = { 0 };
			memcpy(byteArr, &value, sizeof(T));
			is.write(byteArr, sizeof(T));
		}

		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value, void> Read(const char* buffer, T& value, size_t size = sizeof(T), bool endian = true) {
			value = 0;

			// 判断当前机器的字节序
			if (endian) { // 数据是大端字节序
				for (size_t i = 0; i < size; ++i) {
					value |= (static_cast<T>(static_cast<unsigned char>(buffer[i])) << ((size - 1 - i) * 8));
				}

			} else { // 数据是小端字节序
				for (size_t i = 0; i < size; ++i) {
					value |= (static_cast<T>(static_cast<unsigned char>(buffer[i])) << (i * 8));
				}
			}
		}

		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value, void> Write(char * buffer, const T&value, size_t size = sizeof(T), bool endian = true)
		{
			if(endian)
			{
				for (size_t i = 0; i < size; ++i) {
					buffer[i] = static_cast<char>((value >> ((size - 1 - i) * 8)) & 0xFF);
				}
			}
			else
			{
				for (size_t i = 0; i < size; ++i) {
					buffer[i] = static_cast<char>((value >> (i * 8)) & 0xFF);
				}
			}
		}

		inline void Write(std::ostream& os, const std::string& value)
		{
			if (!value.empty())
			{
				os.write(value.c_str(), value.size());
			}
		}

		inline void Write(std::ostream& os, const char* str, size_t size)
		{
			if (str != nullptr && size > 0)
			{
				os.write(str, size);
			}
		}
	}


	class IHeader
	{
	public:
		IHeader() = default;
		virtual ~IHeader() = default;
	public:
		void Add(const IHeader & header);
		void Add(const std::string & k, int v);
		void Add(const std::string & k, long long v);
		void Add(const std::string & k, const std::string & v);
		void Set(const std::string & k, const std::string & v);
	public:
		static int LuaGet(lua_State * L);
		static int LuaToString(lua_State * L);
		static void WriteLua(lua_State *L, const IHeader & header);
	public:
		int GetInt(const std::string & key, int def = 0) const;
		size_t Count() const { return this->mHeader.size(); }
		const auto & GetValue() const { return this->mHeader; }
	public:
		bool Del(const std::string& k);
		bool Del(const std::string& k, int & v);
		bool Del(const std::string& k, std::string & v);
	public:
		bool Has(const std::string &k) const;
		bool Get(const std::string &k, int & v) const;
		bool Get(const std::string &k, long long & v) const;
		bool Get(const std::string &k, std::string & v) const;
		bool IsEqual(const std::string &k, const std::string & v) const;
	protected:
		std::vector<std::pair<std::string, std::string>> mHeader;
	};
}
#endif //APP_IPROTO_H
