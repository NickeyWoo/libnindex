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

template<typename KeyT, typename ValueT, bool Maximum,
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

	virtual ~HeapBase()
	{
		Delete();
	}

	ValueT* Push(KeyT key)
	{
		if(IsFull())
			return NULL;

		size_t curIndex = m_Head->ElementCount++;
		while(curIndex > 0)
		{
			int result = KeyCompare<KeyT>::Compare(key, m_NodeBuffer[ParentIndex(curIndex)].Key);
			if(Maximum) result = 0 - result;

			if(result < 0)
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
		size_t curIndex = 0;
		size_t leftChildIndex = 2 * curIndex + 1;
		while(leftChildIndex <= m_Head->ElementCount)
		{
			int result = 1;
			if(leftChildIndex + 1 <= m_Head->ElementCount)
			{
				result = KeyCompare<KeyT>::Compare(m_NodeBuffer[leftChildIndex + 1].Key, m_NodeBuffer[leftChildIndex].Key);
				if(Maximum)
					result = 0 - result;
			}

			if(result < 0)
			{
				m_NodeBuffer[curIndex] = m_NodeBuffer[leftChildIndex + 1];
				curIndex = leftChildIndex + 1;
			}
			else
			{
				m_NodeBuffer[curIndex] = m_NodeBuffer[leftChildIndex];
				curIndex = leftChildIndex;
			}
			leftChildIndex = 2 * curIndex + 1;
		}
		memset(&m_NodeBuffer[m_Head->ElementCount], 0, sizeof(HeapNode<KeyT, ValueT>));
	}

	void Dump()
	{
		for(size_t i=0; i<m_Head->ElementCount; ++i)
			printf("%s ", KeySerialization<KeyT>::Serialization(m_NodeBuffer[i].Key).c_str());
		printf("\n");
	}

protected:

	inline bool IsFull()
	{
		return (m_Head->ElementCount >= m_Head->BufferSize);
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
	public HeapBase<KeyT, ValueT, false, MinimumHeap>
{
public:
	inline ValueT* Minimum()
	{
		return &this->m_NodeBuffer[0].Value;
	}

};

template<typename KeyT, typename ValueT>
class MaximumHeap :
	public HeapBase<KeyT, ValueT, true, MaximumHeap>
{
public:
	inline ValueT* Maximum()
	{
		return &this->m_NodeBuffer[0].Value;
	}
};


#endif // define __HEAP_HPP__
