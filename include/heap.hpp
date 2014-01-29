/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2014.01.15
 *
*--*/
#ifndef __HEAP_HPP__
#define __HEAP_HPP__

#include <utility>
#include <sys/types.h>
#include "keyutility.hpp"
#include "blocktable.hpp"

template<typename KeyT, typename ValueT>
struct HeapNode
{
	KeyT Key;
	ValueT Value;
} __attribute__((packed));

struct HeapHead
{
	size_t ElementCount;
	size_t BufferSize;
} __attribute__((packed));

template<typename KeyT, typename ValueT, typename CompareT,
			template<typename HeapKeyT, typename HeapValueT> class HeapT>
class HeapBase
{
public:
	static HeapT<KeyT, ValueT> CreateHeap(size_t size)
	{
		HeapT<KeyT, ValueT> heap;
		heap.m_NeedDelete = true;

		size_t sz = GetBufferSize(size);
		heap.m_Head = (HeapHead*)malloc(sz);
		memset(heap.m_Head, 0, sz);

		heap.m_Head->ElementCount = 0;
		heap.m_Head->BufferSize = size;
		heap.m_NodeBuffer = (HeapNode<KeyT, ValueT>*)((char*)heap.m_Head + sizeof(HeapHead));
		return heap;
	}

	static HeapT<KeyT, ValueT> LoadHeap(char* buffer, size_t size)
	{
		HeapT<KeyT, ValueT> heap;
		heap.m_Head = (HeapHead*)buffer;
		heap.m_NodeBuffer = (HeapNode<KeyT, ValueT>*)(buffer + sizeof(HeapHead));
		return heap;
	}

	template<typename StorageT>
	static inline HeapT<KeyT, ValueT> LoadHeap(StorageT storage)
	{
		return LoadHeap(storage.GetBuffer(), storage.GetSize());
	}

	static inline size_t GetBufferSize(size_t size)
	{
		return sizeof(HeapNode<KeyT, ValueT>) * size + sizeof(HeapHead);
	}

	void Delete()
	{
		if(m_Head && m_NeedDelete)
		{
			free(m_Head);
			m_Head = NULL;
			m_NodeBuffer = NULL;
			m_NeedDelete = false;
		}
	}

	HeapBase() :
		m_NeedDelete(false),
		m_Head(NULL),
		m_NodeBuffer(NULL)
	{
	}

	ValueT* Push(KeyT key)
	{
		if(IsFull())
			return NULL;

		size_t curIndex = m_Head->ElementCount++;
		while(curIndex > 0)
		{
			if(CompareT::Compare(m_NodeBuffer[ParentIndex(curIndex)].Key, key) < 0)
			{
				m_NodeBuffer[curIndex] = m_NodeBuffer[ParentIndex(curIndex)];
				curIndex = ParentIndex(curIndex);
			}
			else
			{
				m_NodeBuffer[curIndex].Key = key;
				return &m_NodeBuffer[curIndex].Value;
			}
		}
		m_NodeBuffer[0].Key = key;
		return &m_NodeBuffer[0].Value;
	}

	void Pop()
	{
		if(m_Head->ElementCount == 0)
			return;

		--m_Head->ElementCount;
		m_NodeBuffer[0] = m_NodeBuffer[m_Head->ElementCount];
		memset(&m_NodeBuffer[m_Head->ElementCount], 0, sizeof(HeapNode<KeyT, ValueT>));

		Heapify(0);
	}

	inline size_t Count()
	{
		return m_Head->ElementCount;
	}

	inline void Dump()
	{
		HexDump((const char*)m_Head, sizeof(HeapNode<KeyT, ValueT>)*m_Head->BufferSize+sizeof(HeapHead), NULL);
	}

	void DumpHeap()
	{
		for(size_t i=0; i<m_Head->ElementCount; ++i)
			printf("%s ", KeySerialization<KeyT>::Serialization(m_NodeBuffer[i].Key).c_str());
		printf("\n");
	}

protected:
	size_t GetLargest(size_t index)
	{
		size_t left = LeftChildIndex(index);
		size_t right = left + 1;

		size_t largest = index;
		if(left < m_Head->ElementCount)
		{
			if(CompareT::Compare(m_NodeBuffer[index].Key, m_NodeBuffer[left].Key) > 0)
				largest = index;
			else
				largest = left;

			if(right < m_Head->ElementCount && CompareT::Compare(m_NodeBuffer[right].Key, m_NodeBuffer[largest].Key) > 0)
				largest = right;
		}
		return largest;
	}

	void Heapify(size_t index)
	{
		while(index < m_Head->ElementCount)
		{
			size_t largest = GetLargest(index);
			if(largest == index)
				break;

			HeapNode<KeyT, ValueT> tmp = m_NodeBuffer[index];
			m_NodeBuffer[index] = m_NodeBuffer[largest];
			m_NodeBuffer[largest] = tmp;
			index = largest;
		}
	}

	inline bool IsFull()
	{
		return (m_Head->ElementCount >= m_Head->BufferSize);
	}

	inline size_t LeftChildIndex(size_t index)
	{
		return 2 * index + 1;
	}

	inline size_t ParentIndex(size_t index)
	{
		return (index - 1) / 2;
	}

	bool m_NeedDelete;
	HeapHead* m_Head;
	HeapNode<KeyT, ValueT>* m_NodeBuffer;
};

template<typename KeyT, typename ValueT>
class MinimumHeap :
	public HeapBase<KeyT, ValueT, MinimumHeap<KeyT, ValueT>, MinimumHeap>
{
public:
	inline ValueT* Minimum(KeyT* pKey = NULL)
	{
		if(this->m_Head->ElementCount == 0)
			return NULL;

		if(pKey) *pKey = this->m_NodeBuffer[0].Key;
		return &this->m_NodeBuffer[0].Value;
	}

	inline static int Compare(KeyT key1, KeyT key2)
	{
		return KeyCompare<KeyT>::Compare(key2, key1);
	}
};

template<typename KeyT, typename ValueT>
class MaximumHeap :
	public HeapBase<KeyT, ValueT, MaximumHeap<KeyT, ValueT>, MaximumHeap>
{
public:
	inline ValueT* Maximum(KeyT* pKey = NULL)
	{
		if(this->m_Head->ElementCount == 0)
			return NULL;

		if(pKey) *pKey = this->m_NodeBuffer[0].Key;
		return &this->m_NodeBuffer[0].Value;
	}

	inline static int Compare(KeyT key1, KeyT key2)
	{
		return KeyCompare<KeyT>::Compare(key1, key2);
	}
};


#endif // define __HEAP_HPP__
