/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.19
 *
*--*/
#ifndef __BLOOMFILTER_HPP__
#define __BLOOMFILTER_HPP__

#include <math.h>
#include <utility>
#include <string>
#include <time.h>
#include "utility.hpp"
#include "keyutility.hpp"
#include "storage.hpp"
#include "bitmap.hpp"

#ifndef BLOOMFILTER_DEFAULT_K
	#define BLOOMFILTER_DEFAULT_K		16
#endif

template<typename KeyT>
class BloomFilter
{
public:
	static BloomFilter<KeyT> CreateBloomFilter(size_t size, size_t k = BLOOMFILTER_DEFAULT_K)
	{
		BloomFilter<KeyT> bf;
		bf.m_kNum = k;
		bf.m_Bitmap = Bitmap<uint64_t>::CreateBitmap(size);
		return bf;
	}

	static BloomFilter<KeyT> LoadBloomFilter(char* buffer, size_t size, size_t k = BLOOMFILTER_DEFAULT_K)
	{
		BloomFilter<KeyT> bf;
		bf.m_kNum = k;
		bf.m_Bitmap = Bitmap<uint64_t>::LoadBitmap(buffer, size);
		return bf;
	}

	template<typename StorageT>
	static BloomFilter<KeyT> LoadBloomFilter(StorageT storage, size_t k = BLOOMFILTER_DEFAULT_K)
	{
		BloomFilter<KeyT> bf;
		bf.m_kNum = k;
		bf.m_Bitmap = Bitmap<uint64_t>::LoadBitmap(storage);
		return bf;
	}

	static inline size_t GetBufferSize(size_t n, double pError)
	{
		double sz = 0 - log(pError) * n / pow(log(2), 2);
		return ceil(sz / 8);
	}

	static inline size_t GetK(size_t n, double pError)
	{
		size_t m = GetBufferSize(n, pError) * 8;
		return m * log(2) / n;
	}

	void Delete()
	{
		m_Bitmap.Delete();
	}

	void Add(KeyT key)
	{
		uint64_t* pHashBuffer = NULL;
		HashFunction(key, &pHashBuffer);

		for(size_t i=0; i<m_kNum; ++i)
			m_Bitmap.Set(pHashBuffer[i]);

		free(pHashBuffer);
	}

	bool Contains(KeyT key)
	{
		uint64_t* pHashBuffer = NULL;
		HashFunction(key, &pHashBuffer);

		for(size_t i=0; i<m_kNum; ++i)
		{
			if(!m_Bitmap.Contains(pHashBuffer[i]))
			{
				free(pHashBuffer);
				return false;
			}
		}

		free(pHashBuffer);
		return true;
	}

	void Dump()
	{
		m_Bitmap.Dump();
	}

	BloomFilter() :
		m_kNum(1)
	{
	}

protected:
	void HashFunction(KeyT key, uint64_t** ppBuffer)
	{
		*ppBuffer = (uint64_t*)malloc(m_kNum*sizeof(uint64_t));
		memset(*ppBuffer, 0, m_kNum*sizeof(uint64_t));

		(*ppBuffer)[0] = KeyTranslate<KeyT>::Translate(key);
		for(size_t i=1; i<m_kNum; ++i)
			(*ppBuffer)[i] = (*ppBuffer)[0] + ((i * (*ppBuffer)[i-1]) % 18446744073709551557U);
	}

	size_t m_kNum;
	Bitmap<uint64_t> m_Bitmap;
};


#endif // define __BLOOMFILTER_HPP__
