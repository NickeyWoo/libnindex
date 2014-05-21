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

template<typename KeyT, typename ValueT, typename HeadT>
struct HashNode
{
	HeadT Key;
	ValueT Value;
} __attribute__((packed));

struct HashTableIterator
{
	HashTableIterator();

	size_t	Offset;
	size_t	Seed;
	size_t	BufferIndex;
};

template<typename KeyT, typename ValueT, 
			typename HeadT, typename HashTableT>
class AbstractHashTable
{
public:
	static HashTableT CreateHashTable(Seed seed)
	{
		HashTableT ht;
		ht.m_PrimeBuffer = seed.GetSeedBuffer();
		ht.m_PrimeCount = seed.GetSize();

		for(size_t i=0; i<ht.m_PrimeCount; ++i)
			ht.m_NodeBufferSize += ht.m_PrimeBuffer[i];
		ht.m_NodeBufferSize *= sizeof(HashNode<KeyT, ValueT, HeadT>);

		ht.m_NodeBuffer = (HashNode<KeyT, ValueT, HeadT>*)malloc(ht.m_NodeBufferSize);
		ht.m_NeedDelete = true;
		return ht;
	}

	static HashTableT LoadHashTable(char* buffer, size_t size, Seed seed)
	{
		HashTableT ht;

		ht.m_PrimeBuffer = seed.GetSeedBuffer();
		ht.m_PrimeCount = seed.GetSize();

		ht.m_NodeBuffer = buffer;
		ht.m_NodeBufferSize = size;
		return ht;
	}

	template<typename StorageT>
	static HashTableT LoadHashTable(StorageT storage, Seed seed)
	{
		HashTableT ht;

		ht.m_PrimeBuffer = seed.GetSeedBuffer();
		ht.m_PrimeCount = seed.GetSize();

		ht.m_NodeBuffer = (HashNode<KeyT, ValueT, HeadT>*)storage.GetStorageBuffer();
		ht.m_NodeBufferSize = storage.GetSize();
		return ht;
	}

	static inline size_t GetNodeSize()
	{
		return sizeof(HashNode<KeyT, ValueT, HeadT>);
	}

	static inline size_t GetBufferSize(Seed seed)
	{
		return seed.GetCount() * sizeof(HashNode<KeyT, ValueT, HeadT>);
	}

	void Clear(KeyT key)
	{
		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<m_PrimeCount; ++i)
		{
			size_t pos = (headKey % m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, HeadT>* pNode = &m_NodeBuffer[pos];
			if(pNode->Key.KeyValue == headKey)
			{
				pNode->Key.KeyValue = 0;
				memset(&pNode->Value, 0, sizeof(ValueT));
				break;
			}
			offset += m_PrimeBuffer[i];
		}
	}

	void Delete()
	{
		if(m_PrimeBuffer)
		{
			free(m_PrimeBuffer);
			m_PrimeBuffer = NULL;
		}
		if(m_NeedDelete && m_NodeBuffer)
		{
			free(m_NodeBuffer);
			m_NodeBuffer = NULL;
		}
	}

	void Dump()
	{
		HexDump((char*)m_NodeBuffer, m_NodeBufferSize, NULL);
	}

	AbstractHashTable() :
		m_NeedDelete(false),
		m_PrimeBuffer(NULL),
		m_PrimeCount(0),
		m_NodeBuffer(NULL),
		m_NodeBufferSize(0)
	{
	}

protected:
	bool m_NeedDelete;

	size_t* m_PrimeBuffer;
	size_t m_PrimeCount;

	HashNode<KeyT, ValueT, HeadT>* m_NodeBuffer;
	size_t m_NodeBufferSize;
};

template<typename KeyT, typename ValueT>
class HashTable :
	public AbstractHashTable<KeyT, ValueT, HashNodeHead<KeyT>, HashTable<KeyT, ValueT> >
{
public:
	ValueT* Next(HashTableIterator* pstIterator)
	{
		if(!pstIterator)
			return NULL;

		for(; pstIterator->BufferIndex<this->m_PrimeCount; ++pstIterator->BufferIndex)
		{
			for(; pstIterator->Seed<this->m_PrimeBuffer[pstIterator->BufferIndex]; ++pstIterator->Seed)
			{
				size_t pos = pstIterator->Seed + pstIterator->Offset;
				HashNode<KeyT, ValueT, HashNodeHead<KeyT> >* pNode = &this->m_NodeBuffer[pos];
				if(pNode->Key.KeyValue != 0)
				{
					++pstIterator->Seed;
					return &pNode->Value;
				}
			}
			pstIterator->Offset += this->m_PrimeBuffer[pstIterator->BufferIndex];
			pstIterator->Seed = 0;
		}
		return NULL;
	}

	ValueT* Hash(KeyT key, bool bNew = false)
	{
		HashNode<KeyT, ValueT, HashNodeHead<KeyT> >* pEmptyNode = NULL;

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, HashNodeHead<KeyT> >* pNode = &this->m_NodeBuffer[pos];

			if(pEmptyNode == NULL && pNode->Key.KeyValue == 0)
				pEmptyNode = pNode;

			if(pNode->Key.KeyValue == headKey)
				return &pNode->Value;

			offset += this->m_PrimeBuffer[i];
		}

		if(bNew && pEmptyNode != NULL)
		{
			pEmptyNode->Key.KeyValue = headKey;
			memset(&pEmptyNode->Value, 0, sizeof(ValueT));
			return &pEmptyNode->Value;
		}
		return NULL;
	}

};

template<typename KeyT, typename ValueT, typename TimeProviderT = SecondTimeProvider>
class TimerHashTable :
	public AbstractHashTable<KeyT, ValueT, 
				TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType>, TimerHashTable<KeyT, ValueT, TimeProviderT> >
{
public:
	TimerHashTable() :
		m_DefaultTimeout(0)
	{
	}

	ValueT* Next(HashTableIterator* pstIterator)
	{
		if(!pstIterator)
			return NULL;

		typename TimeProviderT::TimeType now = m_TimeProvider.Now();
		for(; pstIterator->BufferIndex<this->m_PrimeCount; ++pstIterator->BufferIndex)
		{
			for(; pstIterator->Seed<this->m_PrimeBuffer[pstIterator->BufferIndex]; ++pstIterator->Seed)
			{
				size_t pos = pstIterator->Seed + pstIterator->Offset;
				HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pNode = &this->m_NodeBuffer[pos];
				if(pNode->Key.KeyValue != 0 && m_TimeProvider.Compare(pNode->Key.Timestamp, now) > 0)
				{
					++pstIterator->Seed;
					return &pNode->Value;
				}
			}
			pstIterator->Offset += this->m_PrimeBuffer[pstIterator->BufferIndex];
			pstIterator->Seed = 0;
		}
		return NULL;
	}

	ValueT* Hash(KeyT key, bool bNew = false)
	{
		HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pEmptyNode = NULL;
		TimeProviderT provider;
		typename TimeProviderT::TimeType now = provider.Now();

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pNode = &this->m_NodeBuffer[pos];

			if(pEmptyNode == NULL && (pNode->Key.KeyValue == 0 || m_TimeProvider.Compare(pNode->Key.Timestamp, now) <= 0))
				pEmptyNode = pNode;

			if(pNode->Key.KeyValue == headKey)
			{
				if(m_TimeProvider.Compare(pNode->Key.Timestamp, now) <= 0)
					break;
				else
					return &pNode->Value;
			}

			offset += this->m_PrimeBuffer[i];
		}

		if(bNew && pEmptyNode != NULL)
		{
			pEmptyNode->Key.KeyValue = headKey;
			pEmptyNode->Key.Timestamp = m_TimeProvider.After(now, m_DefaultTimeout);
			memset(&pEmptyNode->Value, 0, sizeof(ValueT));
			return &pEmptyNode->Value;
		}
		return NULL;
	}

	typename TimeProviderT::TimeType Expire(KeyT key, typename TimeProviderT::TimeType timeout)
	{
		typename TimeProviderT::TimeType now = m_TimeProvider.Now();

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
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
			offset += this->m_PrimeBuffer[i];
		}
		return 0;
	}

	typename TimeProviderT::TimeType TTL(KeyT key)
	{
		typename TimeProviderT::TimeType now = m_TimeProvider.Now();

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead<KeyT, typename TimeProviderT::TimeType> >* pNode = &this->m_NodeBuffer[pos];

			if(pNode->Key.KeyValue == headKey)
			{
				if(m_TimeProvider.Compare(pNode->Key.Timestamp, now) > 0)
					return m_TimeProvider.Before(pNode->Key.Timestamp, now);
				else
					return 0;
			}
			offset += this->m_PrimeBuffer[i];
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
