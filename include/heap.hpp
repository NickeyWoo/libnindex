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
		m_NodeBuffer[0] = m_NodeBuffer[m_Head->ElementCount];

		size_t index = 0;
		while(true)
		{
			size_t lIndex = LeftChildIndex(index);
			if(KeyCompare<KeyT>::Compare(m_NodeBuffer[index].Key, m_NodeBuffer[lIndex].Key) > 0)
				largest = Maximum?lIndex:index;
			else
				largest = Maximum?index:lIndex;

			if(KeyCompare<KeyT>::Compare(m_NodeBuffer[largest].Key, m_NodeBuffer[rIndex].Key) > 0)
		}
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

	inline size_t LeftChildIndex(size_t index)
	{
		return 2 * index + 1;
	}

	inline size_t ParentIndex(size_t index)
	{
		return (index - 1) / 2;
	}

	size_t TopK(ValueT* buffer, size_t size)
	{
		size_t i = 0;
		for(; i<size && m_Head->ElementCount > 0; ++i)
		{
			buffer[i] = m_NodeBuffer[0].Value;
			Pop();
		}
		return i;
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
		if(this->m_Head->ElementCount == 0)
			return NULL;
		return &this->m_NodeBuffer[0].Value;
	}

	inline size_t Minimum(ValueT* buffer, size_t size)
	{
		return TopK(buffer, size);
	}
};


template<typename KeyT, typename ValueT>
class MaximumHeap :
	public HeapBase<KeyT, ValueT, true, MaximumHeap>
{
public:
	inline ValueT* Maximum()
	{
		if(this->m_Head->ElementCount == 0)
			return NULL;
		return &this->m_NodeBuffer[0].Value;
	}

	inline size_t Maximum(ValueT* buffer, size_t size)
	{
		return TopK(buffer, size);
	}
};


#endif // define __HEAP_HPP__
