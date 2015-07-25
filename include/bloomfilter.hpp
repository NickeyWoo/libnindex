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

#define BLOOMFILTER_MAGIC   "BLOOMFIL"
#define BLOOMFILTER_VERSION 0x0101

struct BloomFilterMeta {
    char cMagic[8];
    uint16_t wVersion;

    uint32_t dwK;

    uint32_t dwReserved[4];
} __attribute__((packed));

template<typename KeyT>
class BloomFilter
{
public:
	static BloomFilter<KeyT> CreateBloomFilter(size_t size, size_t k = BLOOMFILTER_DEFAULT_K)
	{
		BloomFilter<KeyT> bf;
		bf.m_Bitmap = Bitmap<uint64_t, BloomFilterMeta>::CreateBitmap(size * 8);

        bf.m_MetaInfo = bf.m_Bitmap.GetHead();
        if(bf.m_MetaInfo == NULL)
        {
            bf.m_Bitmap.Delete();
            return bf;
        }

        memcpy(bf.m_MetaInfo->cMagic, BLOOMFILTER_MAGIC, 8);
        bf.m_MetaInfo->wVersion = BLOOMFILTER_VERSION;
        bf.m_MetaInfo->dwK = k;
		return bf;
	}

	static BloomFilter<KeyT> LoadBloomFilter(char* buffer, size_t size, size_t k = BLOOMFILTER_DEFAULT_K)
	{
		BloomFilter<KeyT> bf;
		bf.m_Bitmap = Bitmap<uint64_t, BloomFilterMeta>::LoadBitmap(buffer, size);

        bf.m_MetaInfo = bf.m_Bitmap.GetHead();
        if(bf.m_MetaInfo == NULL)
        {
            bf.m_Bitmap.Delete();
            return bf;
        }

        if(memcmp(bf.m_MetaInfo->cMagic, "\0\0\0\0\0\0\0\0", 8) == NULL)
        {
            memcpy(bf.m_MetaInfo->cMagic, BLOOMFILTER_MAGIC, 8);
            bf.m_MetaInfo->wVersion = BLOOMFILTER_VERSION;
            bf.m_MetaInfo->dwK = k;
        }
        else
        {
            if(memcmp(bf.m_MetaInfo->cMagic, BLOOMFILTER_MAGIC, 8) != 0 ||
                bf.m_MetaInfo->wVersion != BLOOMFILTER_VERSION ||
                bf.m_MetaInfo->dwK != k)
            {
                bf.m_Bitmap.Delete();
                bf.m_MetaInfo = NULL;
            }
        }
		return bf;
	}

	template<typename StorageT>
	static BloomFilter<KeyT> LoadBloomFilter(StorageT storage, size_t k = BLOOMFILTER_DEFAULT_K)
	{
		return BloomFilter<KeyT>::LoadBloomFilter(storage.GetStorageBuffer(), storage.GetSize(), k);
	}

	static inline size_t GetBufferSize(size_t count, double pError)
	{
		double sz = -1 * log(pError) * count / pow(log(2), 2);
		return (size_t)ceil(sz / 8);
	}

	static inline size_t GetK(size_t count, double pError)
	{
		size_t m = GetBufferSize(count, pError) * 8;
		return (size_t)(m * log(2) / count);
	}

    inline bool Success()
    {
        return m_Bitmap.Success();
    }

	void Delete()
	{
		m_Bitmap.Delete();
        m_MetaInfo = NULL;
	}

	void Add(KeyT key)
	{
        if(m_MetaInfo == NULL)
            return;

		uint64_t* pHashBuffer = NULL;
		HashFunction(key, &pHashBuffer);

		for(size_t i=0; i<m_MetaInfo->dwK; ++i)
			m_Bitmap.Set(pHashBuffer[i]);

		free(pHashBuffer);
	}

	bool Contains(KeyT key)
	{
        if(m_MetaInfo == NULL)
            return false;

		uint64_t* pHashBuffer = NULL;
		HashFunction(key, &pHashBuffer);

        bool bContains = true;
		for(size_t i=0; i<m_MetaInfo->dwK; ++i)
		{
			if(!m_Bitmap.Contains(pHashBuffer[i]))
			{
                bContains = false;
                break;
			}
		}
		free(pHashBuffer);
		return bContains;
	}

    float Capacity()
    {
        return m_Bitmap.Capacity();
    }

	void Dump()
	{
		m_Bitmap.Dump();
	}

	BloomFilter() :
        m_MetaInfo(NULL)
	{
	}

protected:
	void HashFunction(KeyT key, uint64_t** ppBuffer)
	{
		*ppBuffer = (uint64_t*)malloc(m_MetaInfo->dwK * sizeof(uint64_t));
		memset(*ppBuffer, 0, m_MetaInfo->dwK * sizeof(uint64_t));

		(*ppBuffer)[0] = KeyTranslate<KeyT>::Translate(key);
		for(size_t i=1; i<m_MetaInfo->dwK; ++i)
			(*ppBuffer)[i] = (*ppBuffer)[0] + ((i * (*ppBuffer)[i-1]) % 18446744073709551557U);
	}

	Bitmap<uint64_t, BloomFilterMeta> m_Bitmap;
    BloomFilterMeta* m_MetaInfo;
};


#endif // define __BLOOMFILTER_HPP__
