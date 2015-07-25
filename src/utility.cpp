#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <utility>
#include <vector>
#include <string>
#include "utility.hpp"
#include "city.h"

uint32_t Hash32(const char* ptr, size_t len)
{
#ifdef HASH_USE_MD5
	return MD5_32(ptr, len);
#else
	return CityHash32(ptr, len);
#endif
}

uint64_t Hash64(const char* ptr, size_t len)
{
#ifdef HASH_USE_MD5
	return MD5_64(ptr, len);
#else
	return CityHash64(ptr, len);
#endif
}

uint128_t Hash128(const char* ptr, size_t len)
{
#ifdef HASH_USE_MD5
	return MD5_128(ptr, len);
#else
	return CityHash128(ptr, len);
#endif
}

uint32_t MD5_32(const char* ptr, size_t len)
{
	MD5_CTX ctx;
	if(MD5_Init(&ctx) != 1)
		return 0;
	
	if(MD5_Update(&ctx, ptr, len) != 1)
		return 0;
	
	unsigned char buffer[16];
	if(MD5_Final(buffer, &ctx) != 1)
		return 0;
	
	return ntohl(*((uint32_t*)(buffer + 6)));
}

uint64_t MD5_64(const char* ptr, size_t len)
{
	MD5_CTX ctx;
	if(MD5_Init(&ctx) != 1)
		return 0;
	
	if(MD5_Update(&ctx, ptr, len) != 1)
		return 0;
	
	unsigned char buffer[16];
	if(MD5_Final(buffer, &ctx) != 1)
		return 0;
	
	return ntohll(*((uint64_t*)(buffer + 4)));
}

uint128_t MD5_128(const char* ptr, size_t len)
{
	uint128_t hash;
	MD5_CTX ctx;
	if(MD5_Init(&ctx) != 1)
		return hash;
	
	if(MD5_Update(&ctx, ptr, len) != 1)
		return hash;
	
	unsigned char buffer[16];
	if(MD5_Final(buffer, &ctx) != 1)
		return hash;
	
	hash.first = ntohll(*((uint64_t*)(buffer + 8))); 
	hash.second = ntohll(*((uint64_t*)(buffer)));
	return hash;
}

#define HEXDUMP_PRINTF(...)                                                     \
			if(out!= NULL)		                                                \
			{                                                                   \
				memset(szLineBuffer, 0, 100);                                   \
				snprintf(szLineBuffer, 100, __VA_ARGS__);                       \
				int iLineSize = strlen(szLineBuffer);                           \
				snprintf(pCurOutBuffer, iCurOutBufferLen, "%s", szLineBuffer);  \
				pCurOutBuffer += iLineSize;                                     \
				iCurOutBufferLen -= iLineSize;                                  \
			}                                                                   \
			else                                                                \
				printf(__VA_ARGS__)

#define OCT_TYPE    8
#define DEC_TYPE    10
#define HEX_TYPE    16

int number_printf_size(long long int llNum, int iType)
{
	char buffer[23];
	memset(buffer, 0, 23);

	if(iType == OCT_TYPE)
	{
		sprintf(buffer, "%llo", llNum);
		return strlen(buffer);
	}
	else if(iType == DEC_TYPE)
	{
		sprintf(buffer, "%lld", llNum);
		return strlen(buffer);
	}
	else if(iType == HEX_TYPE)
	{
		sprintf(buffer, "%llx", llNum);
		return strlen(buffer);
	}
	return 0;
}

void HexDump(const char* ptr, size_t len, char** out)
{
	if(ptr == NULL || len == 0)
		return;

	int iLines = len / 16;
	int iMod = len % 16;
	if(iMod != 0)
		++iLines;

	int iLeftAlign = number_printf_size(iLines, HEX_TYPE);
	char szFormat[32];
	memset(szFormat, 0, 32);
	sprintf(szFormat, "%%0%dX0 | ", iLeftAlign);

	char szLineBuffer[100];
	char* pCurOutBuffer = NULL;
	int iOutBufferLen = 0;
	int iCurOutBufferLen = 0;
	if(out != NULL)
	{
		iOutBufferLen = (iLeftAlign + 71) * (iLines + 1) + 1;
		*out = (char*)malloc(iOutBufferLen);
		pCurOutBuffer = *out;
		iCurOutBufferLen = iOutBufferLen;
		memset(*out, 0, iOutBufferLen);
	}

	for(int i=0; i<iLeftAlign; ++i)
		HEXDUMP_PRINTF("_");
	HEXDUMP_PRINTF("__|__0__1__2__3__4__5__6__7__8__9__A__B__C__D__E__F_|_________________\n");

	int iLineNum = 0;
	HEXDUMP_PRINTF(szFormat, iLineNum);

	char szStrBuffer[17];
	memset(szStrBuffer, 0, 17);
	for(size_t i=0; i<len; ++i)
	{
		unsigned char c = (unsigned char)ptr[i];
		HEXDUMP_PRINTF("%02X ", c);

		int idx = i % 16;
		if(c > 31 && c < 127)
			szStrBuffer[idx] = c;
		else
			szStrBuffer[idx] = '.';

		if((i + 1) % 16 == 0)
		{
			++iLineNum;
			HEXDUMP_PRINTF("| %s\n", szStrBuffer);
			if((i + 1) >= len)
				break;
			HEXDUMP_PRINTF(szFormat, iLineNum);
			memset(szStrBuffer, 0, 17);
		}
	}
	if(iMod)
	{
		int iLastNum = 16 - iMod;
		for(int i=0; i<iLastNum; ++i)
		{
			HEXDUMP_PRINTF("   ");
		}
		HEXDUMP_PRINTF("| %s\n", szStrBuffer);
	}
}

uint32_t GetPrime(uint32_t num)
{
	for(uint32_t prime=num; prime>=2; --prime)
	{
		uint32_t mod = 1;
		uint32_t prime_sqrt = (size_t)sqrt(prime);
		for(uint32_t i=2; i<=prime_sqrt; ++i)
		{
			mod = prime % i;
			if(!mod)
				break;
		}
		if(mod)
			return prime;
	}
	return 0;
}

uint32_t GetPrimes(uint32_t num, uint32_t* buffer, uint32_t len)
{
	uint32_t i = 0;
	for(uint32_t prime=num; prime>=2 && i<len; --prime, ++i)
	{
		prime = GetPrime(prime);
		if(prime == 0)
			break;
		buffer[i] = prime;
	}
	return i;
}

Seed::Seed(uint32_t seed, uint32_t count) :
	m_Buffer(NULL),
	m_BufferSize(0)
{
	m_Buffer = (uint32_t*)malloc(sizeof(uint32_t)*count);
	memset(m_Buffer, 0, sizeof(uint32_t)*count);
	m_BufferSize = GetPrimes(seed, m_Buffer, count);
}

void Seed::Release()
{
	if(m_Buffer)
	{
		free(m_Buffer);
		m_Buffer = NULL;
	}
}

uint32_t Seed::GetCount()
{
	uint32_t count = 0;
	for(uint32_t i=0; i<m_BufferSize; ++i)
		count += m_Buffer[i];
	return count;
}







