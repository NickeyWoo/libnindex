/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.19
 *
*--*/
#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#include <utility>

class NullType;
class EmptyType { };

#define ntohll(val)	\
		((uint64_t)ntohl(0xFFFFFFFF&val) << 32 | ntohl((0xFFFFFFFF00000000&val) >> 32))

typedef std::pair<uint64_t, uint64_t> uint128_t;

uint32_t Hash32(const char* ptr, size_t len);
uint64_t Hash64(const char* ptr, size_t len);
uint128_t Hash128(const char* ptr, size_t len);

uint32_t MD5_32(const char* ptr, size_t len);
uint64_t MD5_64(const char* ptr, size_t len);
uint128_t MD5_128(const char* ptr, size_t len);

void HexDump(const char* ptr, size_t len, char** out);

size_t GetPrime(size_t num);
size_t GetPrimes(size_t num, size_t* buffer, size_t len);

class Seed
{
public:
	size_t GetCount();
	void Release();

	inline size_t* GetBuffer()
	{
		return m_Buffer;
	}

	inline size_t GetSize()
	{
		return m_BufferSize;
	}

	Seed(size_t seed, size_t count);

private:
	size_t* m_Buffer;
	size_t m_BufferSize;
};

#endif // define __UTILITY_HPP__
