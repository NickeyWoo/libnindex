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
#include "utility.hpp"
#include "keyutility.hpp"
#include "storage.hpp"

#ifndef TIMERHASHTABLE_TIMEOUT
	#define TIMERHASHTABLE_TIMEOUT	-1
#endif

template<typename KeyT>
struct HashNodeHead
{
	typename KeyTranslate<KeyT>::HeadType KeyValue;
} __attribute__((packed));

template<typename KeyT>
struct TimerHashNodeHead
{
	typename KeyTranslate<KeyT>::HeadType KeyValue;
	time_t Timestamp;
} __attribute__((packed));

template<typename KeyT, typename ValueT, template<typename> class HeadT>
struct HashNode
{
	HeadT<KeyT> Key;
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
			template<typename> class HeadT, template<typename, typename> class HashTableT>
class AbstractHashTable
{
public:
	static HashTableT<KeyT, ValueT> CreateHashTable(size_t seed, size_t count)
	{
		HashTableT<KeyT, ValueT> ht;
		ht.m_NeedDelete = true;

		ht.m_PrimeBuffer = (size_t*)malloc(sizeof(size_t)*count);
		memset(ht.m_PrimeBuffer, 0, sizeof(size_t)*count);
		ht.m_PrimeCount = GetPrimes(seed, ht.m_PrimeBuffer, count);

		for(size_t i=0; i<ht.m_PrimeCount; ++i)
			ht.m_NodeBufferSize += ht.m_PrimeBuffer[i];
		ht.m_NodeBufferSize *= sizeof(HashNode<KeyT, ValueT, HeadT>);

		ht.m_NodeBuffer = (HashNode<KeyT, ValueT, HeadT>*)malloc(ht.m_NodeBufferSize);
		return ht;
	}

	static HashTableT<KeyT, ValueT> LoadHashTable(char* buffer, size_t size, Seed seed)
	{
		HashTableT<KeyT, ValueT> ht;

		ht.m_PrimeBuffer = seed.GetBuffer();
		ht.m_PrimeCount = seed.GetSize();

		ht.m_NodeBuffer = buffer;
		ht.m_NodeBufferSize = size;
		return ht;
	}

	template<typename StorageT>
	static HashTableT<KeyT, ValueT> LoadHashTable(StorageT storage, Seed seed)
	{
		HashTableT<KeyT, ValueT> ht;

		ht.m_PrimeBuffer = seed.GetBuffer();
		ht.m_PrimeCount = seed.GetSize();

		ht.m_NodeBuffer = (HashNode<KeyT, ValueT, HeadT>*)storage.GetBuffer();
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
		if(m_NeedDelete && m_PrimeBuffer)
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
	public AbstractHashTable<KeyT, ValueT, HashNodeHead, HashTable>
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
				HashNode<KeyT, ValueT, HashNodeHead>* pNode = &this->m_NodeBuffer[pos];
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
		HashNode<KeyT, ValueT, HashNodeHead>* pEmptyNode = NULL;

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, HashNodeHead>* pNode = &this->m_NodeBuffer[pos];

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

template<typename KeyT, typename ValueT>
class TimerHashTable :
	public AbstractHashTable<KeyT, ValueT, TimerHashNodeHead, TimerHashTable>
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

		time_t now = time(NULL);
		for(; pstIterator->BufferIndex<this->m_PrimeCount; ++pstIterator->BufferIndex)
		{
			for(; pstIterator->Seed<this->m_PrimeBuffer[pstIterator->BufferIndex]; ++pstIterator->Seed)
			{
				size_t pos = pstIterator->Seed + pstIterator->Offset;
				HashNode<KeyT, ValueT, TimerHashNodeHead>* pNode = &this->m_NodeBuffer[pos];
				if(pNode->Key.KeyValue != 0 && pNode->Key.Timestamp > now)
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
		HashNode<KeyT, ValueT, TimerHashNodeHead>* pEmptyNode = NULL;
		time_t now = time(NULL);

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead>* pNode = this->m_NodeBuffer[pos];

			if(pEmptyNode == NULL && (pNode->Key.KeyValue == 0 || pNode->Key.Timestamp <= now))
				pEmptyNode = pNode;

			if(pNode->Key.KeyValue == headKey)
			{
				if(pNode->Key.Timestamp <= now)
					break;
				else
					return &pNode->Value;
			}

			offset += this->m_PrimeBuffer[i];
		}

		if(bNew && pEmptyNode != NULL)
		{
			pEmptyNode->Key.KeyValue = headKey;
			pEmptyNode->Key.Timestamp = m_DefaultTimeout==0?TIMERHASHTABLE_TIMEOUT:(now + m_DefaultTimeout);
			memset(&pEmptyNode->Value, 0, sizeof(ValueT));
			return &pEmptyNode->Value;
		}
		return NULL;
	}

	time_t Expire(KeyT key, time_t timeout)
	{
		time_t now = time(NULL);

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead>* pNode = this->m_NodeBuffer[pos];

			if(pNode->Key.KeyValue == headKey)
			{
				if(pNode->Key.Timestamp > now)
				{
					time_t last = pNode->Key.Timestamp;
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

	time_t TTL(KeyT key)
	{
		time_t now = time(NULL);

		typename KeyTranslate<KeyT>::HeadType headKey = KeyTranslate<KeyT>::Translate(key);
		size_t offset = 0;
		for(size_t i=0; i<this->m_PrimeCount; ++i)
		{
			size_t pos = (headKey % this->m_PrimeBuffer[i] + offset);
			HashNode<KeyT, ValueT, TimerHashNodeHead>* pNode = this->m_NodeBuffer[pos];

			if(pNode->Key.KeyValue == headKey)
			{
				if(pNode->Key.Timestamp > now)
					return pNode->Key.Timestamp - now;
				else
					return 0;
			}
			offset += this->m_PrimeBuffer[i];
		}
		return 0;
	}

	inline time_t GetDefaultTimeout()
	{
		return m_DefaultTimeout;
	}

	inline time_t SetDefaultTimeout(time_t timeout)
	{
		time_t last = m_DefaultTimeout;
		m_DefaultTimeout = timeout;
		return last;
	}

protected:
	time_t	m_DefaultTimeout;
};

#endif // define __HASHTABLE_HPP__
