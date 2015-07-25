/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.19
 *
*--*/
#ifndef __HASHTABLE_HPP__
#define __HASHTABLE_HPP__

#include <utility>
#include <string>
#include <time.h>
#include <sys/time.h>
#include "utility.hpp"
#include "keyutility.hpp"
#include "storage.hpp"

#ifndef TIMERHASHTABLE_TIMEOUT
	#define TIMERHASHTABLE_TIMEOUT	-1
#endif

struct SecondTimeProvider
{
	typedef time_t TimeType;

	inline TimeType Now()
	{
		return time(NULL);
	}

	inline TimeType Before(TimeType v1, TimeType v2)
	{
		return v1 - v2;
	}

	inline TimeType After(TimeType v1, TimeType v2)
	{
		return v1 + v2;
	}

	inline int Compare(TimeType v1, TimeType v2)
	{
		if(v1 > v2)
			return 1;
		else if(v1 < v2)
			return -1;
		else
			return 0;
	}
};

struct MicroSecondTimeProvider
{
	typedef timeval TimeType;

	inline TimeType Now()
	{
		timeval tv;
		gettimeofday(&tv, NULL);
		return tv;
	}

	inline TimeType Before(TimeType v1, TimeType v2)
	{
		timeval tv;
		timersub(&v1, &v2, &tv);
		return tv;
	}

	inline TimeType After(TimeType v1, TimeType v2)
	{
		timeval tv;
		timeradd(&v1, &v2, &tv);
		return tv;
	}

	inline int Compare(TimeType v1, TimeType v2)
	{
		if(v1.tv_sec > v2.tv_sec)
			return 1;
		else if(v1.tv_sec < v2.tv_sec)
			return -1;
		else
			if(v1.tv_usec > v2.tv_usec)
				return 1;
			else if(v1.tv_usec < v2.tv_usec)
				return -1;
			else
				return 0;
	}
};

#define HASHTABLE_MAGIC         "HASHTABL"
#define TIMERHASHTABLE_MAGIC    "TIMEHASH"
#define HASHTABLE_VERSION       0x0101

template<typename HeadT>
struct HashTableMetaInfo {
    char cMagic[8];
    uint16_t wVersion;

    uint32_t dwHeadSize;
    uint64_t ddwMemSize;

    uint32_t dwTotal;
    uint32_t dwUsed;

    uint32_t dwReserved[4];

    HeadT stHead;

    uint8_t cSeedCount;
    uint32_t dwSeedBuffer[0];
} __attribute__((packed));
template<>
struct HashTableMetaInfo<void> {
    char cMagic[8];
    uint16_t wVersion;

    uint32_t dwHeadSize;
    uint64_t ddwMemSize;

    uint32_t dwTotal;
    uint32_t dwUsed;

    uint32_t dwReserved[4];

    uint8_t cSeedCount;
    uint32_t dwSeedBuffer[0];
} __attribute__((packed));

template<typename KeyT>
struct HashNodeHead
{
	typename KeyTranslate<KeyT>::HeadType KeyValue;
} __attribute__((packed));

template<typename KeyT, typename TimeType>
struct TimerHashNodeHead
{
	typename KeyTranslate<KeyT>::HeadType KeyValue;
	TimeType Timestamp;
} __attribute__((packed));

template<typename KeyT, typename ValueT, typename NodeHeadT>
struct HashNode
{
	NodeHeadT Key;
	ValueT Value;
} __attribute__((packed));

struct HashTableIterator
{
	HashTableIterator();

	size_t	Offset;
	size_t	Seed;
	size_t	BufferIndex;
};

template<typename KeyT, typename ValueT, typename HeadT>
class HashTable;
template<typename KeyT, typename ValueT, typename HeadT, typename TimeProviderT>
class TimerHashTable;

template<typename T>
struct HashFunction;
template<typename KeyT, typename ValueT, typename HeadT>
struct HashFunction<HashTable<KeyT, ValueT, HeadT> > {
    static const char* Magic()
    {
        return HASHTABLE_MAGIC;
    }
};
template<typename KeyT, typename ValueT, typename HeadT, typename TimeProviderT>
struct HashFunction<TimerHashTable<KeyT, ValueT, HeadT, TimeProviderT> > {
    static const char* Magic()
    {
        return TIMERHASHTABLE_MAGIC;
    }
};

template<typename KeyT, typename ValueT, typename NodeHeadT, typename HashTableT, typename HeadT>
class AbstractHashTable
{
public:
	static HashTableT CreateHashTable(Seed& seed)
	{
        HashTableT ht;

        if(seed.GetSize() > 250)
            return ht;

        uint32_t nodeCount = seed.GetCount();
        uint32_t headSize = sizeof(HashTableMetaInfo<HeadT>) + seed.GetSize() * sizeof(uint32_t);
        size_t bufferSize = headSize + nodeCount * sizeof(HashNode<KeyT, ValueT, NodeHeadT>);

        ht.m_TableMetaInfo = (HashTableMetaInfo<HeadT>*)malloc(bufferSize);
        if(!ht.m_TableMetaInfo)
            return ht;

		ht.m_NeedDelete = true;

        memcpy(ht.m_TableMetaInfo->cMagic, HashFunction<HashTableT>::Magic(), 8);
        ht.m_TableMetaInfo->wVersion = HASHTABLE_VERSION;
        ht.m_TableMetaInfo->dwHeadSize = headSize;
        ht.m_TableMetaInfo->ddwMemSize = bufferSize;
        ht.m_TableMetaInfo->dwTotal = nodeCount;
        ht.m_TableMetaInfo->dwUsed = 0;

        ht.m_TableMetaInfo->cSeedCount = seed.GetSize();
        for(uint32_t i=0; i<seed.GetSize(); ++i)
            ht.m_TableMetaInfo->dwSeedBuffer[i] = seed.GetSeed(i);

		ht.m_NodeBuffer = (HashNode<KeyT, ValueT, NodeHeadT>*)((char*)ht.m_TableMetaInfo + headSize);
		return ht;
	}

	static HashTableT LoadHashTable(char* buffer, size_t size, Seed& seed)
	{
		HashTableT ht;
        ht.Initialize(buffer, size, seed);
        return ht;
	}

	template<typename StorageT>
	static HashTableT LoadHashTable(StorageT storage, Seed& seed)
	{
		HashTableT ht;
        ht.Initialize(storage.GetStorageBuffer(), storage.GetSize(), seed);
		return ht;
	}

    inline bool Success()
    {
        return m_TableMetaInfo != NULL && m_NodeBuffer != NULL;
    }

    bool Initialize(char* buffer, size_t size, Seed& seed)
    {
        m_TableMetaInfo = (HashTableMetaInfo<HeadT>*)buffer;

        if(memcmp(m_TableMetaInfo->cMagic, "\0\0\0\0\0\0\0\0", 8) == 0)
        {
            // uninitialize storage
            memcpy(m_TableMetaInfo->cMagic, HashFunction<HashTableT>::Magic(), 8);
            m_TableMetaInfo->wVersion = HASHTABLE_VERSION;

            m_TableMetaInfo->dwTotal = seed.GetCount();
            m_TableMetaInfo->dwUsed = 0;

            m_TableMetaInfo->dwHeadSize = sizeof(HashTableMetaInfo<HeadT>) + seed.GetSize() * sizeof(uint32_t);
            m_TableMetaInfo->ddwMemSize = m_TableMetaInfo->dwHeadSize + sizeof(HashNode<KeyT, ValueT, NodeHeadT>) * m_TableMetaInfo->dwTotal;

            m_TableMetaInfo->cSeedCount = seed.GetSize();
            for(uint32_t i=0; i<seed.GetSize(); ++i)
                m_TableMetaInfo->dwSeedBuffer[i] = seed.GetSeed(i);

            if(m_TableMetaInfo->ddwMemSize != size)
            {
                m_TableMetaInfo = NULL;
                return false;
            }

            m_NodeBuffer = (HashNode<KeyT, ValueT, NodeHeadT>*)(buffer + m_TableMetaInfo->dwHeadSize);
            return true;
        }
        else
        {
            if(memcmp(m_TableMetaInfo->cMagic, HashFunction<HashTableT>::Magic(), 8) != 0 ||
                m_TableMetaInfo->wVersion != HASHTABLE_VERSION ||
                m_TableMetaInfo->cSeedCount != seed.GetSize() ||
                m_TableMetaInfo->dwTotal != seed.GetCount() ||
                m_TableMetaInfo->dwHeadSize != (sizeof(HashTableMetaInfo<HeadT>) + seed.GetSize() * sizeof(uint32_t)) ||
                m_TableMetaInfo->ddwMemSize != size)
            {
                m_TableMetaInfo = NULL;
                return false;
            }

            for(uint32_t i=0; i<seed.GetSize(); ++i)
            {
                if(m_TableMetaInfo->dwSeedBuffer[i] != seed.GetSeed(i))
                {
                    m_TableMetaInfo = NULL;
                    return false;
                }
            }

            m_NodeBuffer = (HashNode<KeyT, ValueT, NodeHeadT>*)(buffer + m_TableMetaInfo->dwHeadSize);
            return true;
        }
    }

	static inline size_t GetNodeSize()
	{
		return sizeof(HashNode<KeyT, ValueT, NodeHeadT>);
	}

	static inline size_t GetBufferSize(Seed& seed)
	{
        uint32_t headSize = sizeof(HashTableMetaInfo<HeadT>) + seed.GetSize() * sizeof(uint32_t);
        return headSize + seed.GetCount() * sizeof(HashNode<KeyT, ValueT, NodeHeadT>);
	}

    inline HeadT* GetHead()
    {
        if(!m_TableMetaInfo)
            return NULL;
        return &m_TableMetaInfo->stHead;
    }

	void Clear(KeyT key)
	{
        if(!m_TableMetaInfo)
            return;

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(uint8_t i=0; i<m_TableMetaInfo->cSeedCount; ++i)
		{
			size_t pos = (headKey % m_TableMetaInfo->dwSeedBuffer[i] + offset);
			HashNode<KeyT, ValueT, NodeHeadT>* pNode = &m_NodeBuffer[pos];
			if(pNode->Key.KeyValue == headKey)
			{
				pNode->Key.KeyValue = 0;
				memset(&pNode->Value, 0, sizeof(ValueT));

                --m_TableMetaInfo->dwUsed;
				break;
			}
			offset += m_TableMetaInfo->dwSeedBuffer[i];
		}
	}

	void Delete()
	{
		if(m_NeedDelete && m_TableMetaInfo)
			free(m_TableMetaInfo);

        m_TableMetaInfo = NULL;
        m_NodeBuffer = NULL;
	}

    float Capacity()
    {
        if(!m_TableMetaInfo) 
            return 1;
        return (float)m_TableMetaInfo->dwUsed / m_TableMetaInfo->dwTotal;
    }

	void Dump()
	{
        if(!m_TableMetaInfo)
            return;
        HexDump((char*)m_TableMetaInfo, m_TableMetaInfo->ddwMemSize, NULL);
	}

	AbstractHashTable() :
		m_NeedDelete(false),
		m_TableMetaInfo(NULL),
		m_NodeBuffer(NULL)
	{
	}

protected:
	bool m_NeedDelete;

    HashTableMetaInfo<HeadT>* m_TableMetaInfo;
	HashNode<KeyT, ValueT, NodeHeadT>* m_NodeBuffer;
};

template<typename KeyT, typename ValueT, typename HeadT = void>
class HashTable :
	public AbstractHashTable<KeyT, ValueT, HashNodeHead<KeyT>, HashTable<KeyT, ValueT, HeadT>, HeadT>
{
public:
	ValueT* Next(HashTableIterator* pstIterator)
	{
		if(!pstIterator || !this->m_TableMetaInfo || !this->m_NodeBuffer)
			return NULL;

		for(; pstIterator->BufferIndex<this->m_TableMetaInfo->cSeedCount; ++pstIterator->BufferIndex)
		{
			for(; pstIterator->Seed<this->m_TableMetaInfo->dwSeedBuffer[pstIterator->BufferIndex]; ++pstIterator->Seed)
			{
				size_t pos = pstIterator->Seed + pstIterator->Offset;
				HashNode<KeyT, ValueT, HashNodeHead<KeyT> >* pNode = &this->m_NodeBuffer[pos];
				if(pNode->Key.KeyValue != 0)
				{
					++pstIterator->Seed;
					return &pNode->Value;
				}
			}
			pstIterator->Offset += this->m_TableMetaInfo->dwSeedBuffer[pstIterator->BufferIndex];
			pstIterator->Seed = 0;
		}
		return NULL;
	}

	ValueT* Hash(KeyT key, bool bNew = false)
	{
        if(!this->m_TableMetaInfo || !this->m_NodeBuffer)
            return NULL;

		HashNode<KeyT, ValueT, HashNodeHead<KeyT> >* pEmptyNode = NULL;

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(uint8_t i=0; i<this->m_TableMetaInfo->cSeedCount; ++i)
		{
			size_t pos = (headKey % this->m_TableMetaInfo->dwSeedBuffer[i] + offset);
			HashNode<KeyT, ValueT, HashNodeHead<KeyT> >* pNode = &this->m_NodeBuffer[pos];

			if(pEmptyNode == NULL && pNode->Key.KeyValue == 0)
				pEmptyNode = pNode;

			if(pNode->Key.KeyValue == headKey)
				return &pNode->Value;

			offset += this->m_TableMetaInfo->dwSeedBuffer[i];
		}

		if(bNew && pEmptyNode != NULL)
		{
			pEmptyNode->Key.KeyValue = headKey;
			memset(&pEmptyNode->Value, 0, sizeof(ValueT));

            ++this->m_TableMetaInfo->dwUsed;
			return &pEmptyNode->Value;
		}
		return NULL;
	}
};

template<typename KeyT, typename ValueT, typename HeadT = void, typename TimeProviderT = SecondTimeProvider>
class TimerHashTable :
	public AbstractHashTable<KeyT, ValueT, 
				TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType>, 
                TimerHashTable<KeyT, ValueT, HeadT, TimeProviderT>,
                HeadT>
{
public:
	TimerHashTable() :
		m_DefaultTimeout(0)
	{
	}

	ValueT* Next(HashTableIterator* pstIterator)
	{
		if(!pstIterator || !this->m_TableMetaInfo || !this->m_NodeBuffer)
			return NULL;

		typename TimeProviderT::TimeType now = m_TimeProvider.Now();
		for(; pstIterator->BufferIndex<this->m_TableMetaInfo->cSeedCount; ++pstIterator->BufferIndex)
		{
			for(; pstIterator->Seed<this->m_TableMetaInfo->dwSeedBuffer[pstIterator->BufferIndex]; ++pstIterator->Seed)
			{
				size_t pos = pstIterator->Seed + pstIterator->Offset;
				HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pNode = &this->m_NodeBuffer[pos];
				if(pNode->Key.KeyValue != 0 && m_TimeProvider.Compare(pNode->Key.Timestamp, now) > 0)
				{
					++pstIterator->Seed;
					return &pNode->Value;
				}
			}
			pstIterator->Offset += this->m_TableMetaInfo->dwSeedBuffer[pstIterator->BufferIndex];
			pstIterator->Seed = 0;
		}
		return NULL;
	}

	ValueT* Hash(KeyT key, bool bNew = false)
	{
		if(!this->m_TableMetaInfo || !this->m_NodeBuffer)
			return NULL;

		HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pEmptyNode = NULL;
		TimeProviderT provider;
		typename TimeProviderT::TimeType now = provider.Now();

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;

        bool bIsExpire = false;
		for(uint8_t i=0; i<this->m_TableMetaInfo->cSeedCount; ++i)
		{
			size_t pos = (headKey % this->m_TableMetaInfo->dwSeedBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pNode = &this->m_NodeBuffer[pos];

			if(pEmptyNode == NULL)
            {
                if(pNode->Key.KeyValue == 0)
                    pEmptyNode = pNode;
                else if(m_TimeProvider.Compare(pNode->Key.Timestamp, now) <= 0)
                {
                    pEmptyNode = pNode;
                    bIsExpire = true;
                }
            }

			if(pNode->Key.KeyValue == headKey)
			{
				if(m_TimeProvider.Compare(pNode->Key.Timestamp, now) <= 0)
					break;
				else
					return &pNode->Value;
			}

			offset += this->m_TableMetaInfo->dwSeedBuffer[i];
		}

		if(bNew && pEmptyNode != NULL)
		{
			pEmptyNode->Key.KeyValue = headKey;
			pEmptyNode->Key.Timestamp = m_TimeProvider.After(now, m_DefaultTimeout);
			memset(&pEmptyNode->Value, 0, sizeof(ValueT));

            if(!bIsExpire)
                ++this->m_TableMetaInfo->dwUsed;
			return &pEmptyNode->Value;
		}
		return NULL;
	}

	typename TimeProviderT::TimeType Expire(KeyT key, typename TimeProviderT::TimeType timeout)
	{
		if(!this->m_TableMetaInfo || !this->m_NodeBuffer)
			return NULL;

		typename TimeProviderT::TimeType now = m_TimeProvider.Now();

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(uint8_t i=0; i<this->m_TableMetaInfo->cSeedCount; ++i)
		{
			size_t pos = (headKey % this->m_TableMetaInfo->dwSeedBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pNode = &this->m_NodeBuffer[pos];

			if(pNode->Key.KeyValue == headKey)
			{
				if(m_TimeProvider.Compare(pNode->Key.Timestamp, now) > 0)
				{
					typename TimeProviderT::TimeType last = pNode->Key.Timestamp;
					pNode->Key.Timestamp = timeout;
					return last;
				}
				else
					return 0;
			}
			offset += this->m_TableMetaInfo->dwSeedBuffer[i];
		}
		return 0;
	}

	typename TimeProviderT::TimeType TTL(KeyT key)
	{
		if(!this->m_TableMetaInfo || !this->m_NodeBuffer)
			return NULL;

		typename TimeProviderT::TimeType now = m_TimeProvider.Now();

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(uint8_t i=0; i<this->m_TableMetaInfo->cSeedCount; ++i)
		{
			size_t pos = (headKey % this->m_TableMetaInfo->dwSeedBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pNode = &this->m_NodeBuffer[pos];

			if(pNode->Key.KeyValue == headKey)
			{
				if(m_TimeProvider.Compare(pNode->Key.Timestamp, now) > 0)
					return m_TimeProvider.Before(pNode->Key.Timestamp, now);
				else
					return 0;
			}
			offset += this->m_TableMetaInfo->dwSeedBuffer[i];
		}
		return 0;
	}

	inline typename TimeProviderT::TimeType GetDefaultTimeout()
	{
		return m_DefaultTimeout;
	}

	inline typename TimeProviderT::TimeType SetDefaultTimeout(typename TimeProviderT::TimeType timeout)
	{
		typename TimeProviderT::TimeType last = m_DefaultTimeout;
		m_DefaultTimeout = timeout;
		return last;
	}

protected:
	TimeProviderT m_TimeProvider;
	typename TimeProviderT::TimeType m_DefaultTimeout;
};

#endif // define __HASHTABLE_HPP__
