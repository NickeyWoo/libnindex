/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.24
 *
*--*/
#ifndef __BLOCKTABLE_HPP__
#define __BLOCKTABLE_HPP__

#include <boost/static_assert.hpp>

#define	BLOCKTABLE_FLAG_INITIALIZE		1

#define	BLOCK_FLAG_ACTIVE				1

template<typename ValueT, typename IndexT>
struct Block
{
	ValueT Value;
	IndexT Next;
	uint8_t Flags;
} __attribute__((packed));

template<typename HeadT, typename IndexT>
struct BlockHead
{
	HeadT Head;
	IndexT EmptyIndex;
	uint64_t Flags;
} __attribute__((packed));

template<typename IndexT>
struct BlockHead<void, IndexT>
{
	IndexT EmptyIndex;
	uint64_t Flags;
} __attribute__((packed));

template<typename ValueT, typename HeadT = void, typename IndexT = uint32_t>
class BlockTable
{
public:
	typedef IndexT BlockIndexType;

	static BlockTable<ValueT, HeadT, IndexT> CreateBlockTable(IndexT size)
	{
		BlockTable<ValueT, HeadT, IndexT> bt;
		bt.m_NeedDelete = true;
		bt.m_Count = size;

		size_t buffer_size = sizeof(BlockHead<HeadT, IndexT>) + sizeof(Block<ValueT, IndexT>) * bt.m_Count;
		bt.m_BlockHead = (BlockHead<HeadT, IndexT>*)malloc(buffer_size);
		memset(bt.m_BlockHead, 0, buffer_size);
		bt.m_BlockHead->Flags = BLOCKTABLE_FLAG_INITIALIZE;
		bt.m_BlockHead->EmptyIndex = 1;

		bt.m_BlockBuffer = (Block<ValueT, IndexT>*)((char*)bt.m_BlockHead + sizeof(BlockHead<HeadT, IndexT>));
		return bt;
	}

	static BlockTable<ValueT, HeadT, IndexT> LoadBlockTable(char* buffer, size_t size)
	{
		BlockTable<ValueT, HeadT, IndexT> bt;
		bt.m_BlockHead = (BlockHead<HeadT, IndexT>*)buffer;
		bt.m_BlockBuffer = (Block<ValueT, IndexT>*)((char*)bt.m_BlockHead + sizeof(BlockHead<HeadT, IndexT>));
		bt.m_Count  = (size - sizeof(BlockHead<HeadT, IndexT>)) / sizeof(Block<ValueT, IndexT>);
		if((bt.m_BlockHead->Flags & BLOCKTABLE_FLAG_INITIALIZE) != BLOCKTABLE_FLAG_INITIALIZE)
		{
			bt.m_BlockHead->Flags = BLOCKTABLE_FLAG_INITIALIZE;
			bt.m_BlockHead->EmptyIndex = 1;
		}
		return bt;
	}

	template<typename StorageT>
	static inline BlockTable<ValueT, HeadT, IndexT> LoadBlockTable(StorageT storage)
	{
		return LoadBlockTable(storage.GetBuffer(), storage.GetSize());
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return sizeof(BlockHead<HeadT, IndexT>) + sizeof(Block<ValueT, IndexT>) * size;
	}

	void Delete()
	{
		if(m_NeedDelete && m_BlockHead)
		{
			free(m_BlockHead);
			m_BlockHead = NULL;
		}
	}

	inline HeadT* GetHead()
	{
		return &m_BlockHead->Head;
	}

	inline Block<ValueT, IndexT>* GetBlockBuffer(Block<ValueT, IndexT>** ppBuffer, IndexT* pCount)
	{
		*ppBuffer = m_BlockBuffer;
		*pCount = m_Count;
		return *ppBuffer;
	}

	IndexT AllocateBlock()
	{
		if(m_BlockHead->EmptyIndex == 0 || m_BlockHead->EmptyIndex > m_Count)
			return 0;

		IndexT newBlockId = m_BlockHead->EmptyIndex;
		Block<ValueT, IndexT>* pBlock = &m_BlockBuffer[newBlockId - 1];
		if(pBlock->Next == 0)
			++m_BlockHead->EmptyIndex;
		else
			m_BlockHead->EmptyIndex = pBlock->Next;

		pBlock->Next = 0;
		pBlock->Flags = BLOCK_FLAG_ACTIVE;
		memset(&pBlock->Value, 0, sizeof(ValueT));
		return newBlockId;
	}

	void ReleaseBlock(IndexT id)
	{
		if(id == 0 || id > m_Count)
			return;

		Block<ValueT, IndexT>* pBlock = &m_BlockBuffer[id - 1];
		if((pBlock->Flags & BLOCK_FLAG_ACTIVE) != BLOCK_FLAG_ACTIVE)
			return;

		pBlock->Next = m_BlockHead->EmptyIndex;
		pBlock->Flags = ~BLOCK_FLAG_ACTIVE;
		m_BlockHead->EmptyIndex = id;
	}

	inline IndexT GetBlockID(ValueT* pVal)
	{
		if(pVal == NULL)
			return 0;
		else
			return ((char*)pVal - (char*)m_BlockBuffer) / sizeof(Block<ValueT, IndexT>) + 1;
	}

	inline ValueT* GetBlock(IndexT id)
	{
		if(id == 0 || id > m_Count)
			return NULL;
		return &(m_BlockBuffer[id - 1].Value);
	}

	inline ValueT* operator[](IndexT id)
	{
		return GetBlock(id);
	}

	void Dump()
	{
		printf("Head Buffer:\n");
		HexDump((const char*)m_BlockHead, sizeof(BlockHead<HeadT, IndexT>), NULL);
		printf("Data Buffer:\n");
		HexDump((const char*)m_BlockBuffer, sizeof(Block<ValueT, IndexT>) * m_Count, NULL);
	}

	BlockTable() :
		m_NeedDelete(false),
		m_BlockHead(NULL),
		m_BlockBuffer(NULL),
		m_Count(0)
	{
	}

protected:
	bool m_NeedDelete;

	BlockHead<HeadT, IndexT>* m_BlockHead;

	Block<ValueT, IndexT>* m_BlockBuffer;
	IndexT m_Count;
};


/////////////////////////////////////////////////////////////////////////////////////////////////
// MultiBlockTable

template<typename TypeListT, typename IndexT, uint32_t IndexValue>
struct GetTypeBufferSize
{
	static size_t Size(std::vector<IndexT>& vSize);
};
template<typename IndexT, uint32_t IndexValue>
struct GetTypeBufferSize<NullType, IndexT, IndexValue>
{
	static inline size_t Size(std::vector<IndexT>& vSize)
	{
		return 0;
	}
};
template<typename Type1, typename Type2, typename IndexT, uint32_t IndexValue>
struct GetTypeBufferSize<TypeList<Type1, Type2>, IndexT, IndexValue>
{
	static inline size_t Size(std::vector<IndexT>& vSize)
	{
		return (sizeof(Block<Type1, IndexT>) * vSize[IndexValue])
					+ GetTypeBufferSize<Type2, IndexT, IndexValue + 1>::Size(vSize);
	}
};

template<typename TypeListT, typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBufferOffset
{
	static size_t Offset(IndexT index, std::vector<IndexT>& vSize);
};
template<typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBufferOffset<NullType, TypeT, IndexT, IndexValue>
{
	static inline size_t Offset(IndexT index, std::vector<IndexT>& vSize)
	{
		return 0;
	}
};
template<typename Type2, typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBufferOffset<TypeList<TypeT, Type2>, TypeT, IndexT, IndexValue>
{
	static inline size_t Offset(IndexT index, std::vector<IndexT>& vSize)
	{
		return sizeof(Block<TypeT, IndexT>) * (index - 1);
	}
};
template<typename Type1, typename Type2, typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBufferOffset<TypeList<Type1, Type2>, TypeT, IndexT, IndexValue>
{
	static inline size_t Offset(IndexT index, std::vector<IndexT>& vSize)
	{
		return (sizeof(Block<Type1, IndexT>) * vSize[IndexValue])
					+ GetBufferOffset<Type2, TypeT, IndexT, IndexValue + 1>::Offset(index, vSize);
	}
};

template<typename TypeListT, typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBlockNodeID
{
	static IndexT ID(char* buffer, const TypeT* pValue, std::vector<IndexT>& vSize);
};
template<typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBlockNodeID<NullType, TypeT, IndexT, IndexValue>
{
	static inline IndexT ID(char* buffer, const TypeT* pValue, std::vector<IndexT>& vSize)
	{
		return 0;
	}
};
template<typename Type2, typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBlockNodeID<TypeList<TypeT, Type2>, TypeT, IndexT, IndexValue>
{
	static inline IndexT ID(char* buffer, const TypeT* pValue, std::vector<IndexT>& vSize)
	{
		return ((char*)pValue - buffer) / sizeof(Block<TypeT, IndexT>) + 1;
	}
};
template<typename Type1, typename Type2, typename TypeT, typename IndexT, uint32_t IndexValue>
struct GetBlockNodeID<TypeList<Type1, Type2>, TypeT, IndexT, IndexValue>
{
	static inline IndexT ID(char* buffer, const TypeT* pValue, std::vector<IndexT>& vSize)
	{
		return GetBlockNodeID<Type2, TypeT, IndexT, IndexValue + 1>::ID((buffer + sizeof(Block<Type1, IndexT>) * vSize[IndexValue]), pValue, vSize);
	}
};

template<typename TypeListT, typename IndexT, uint32_t IndexValue>
struct DumpTypeBuffer
{
	static void Dump(const char* buffer, std::vector<IndexT>& vSize);
};
template<typename IndexT, uint32_t IndexValue>
struct DumpTypeBuffer<NullType, IndexT, IndexValue>
{
	static void Dump(const char* buffer, std::vector<IndexT>& vSize)
	{
	}
};
template<typename Type1, typename Type2, typename IndexT, uint32_t IndexValue>
struct DumpTypeBuffer<TypeList<Type1, Type2>, IndexT, IndexValue>
{
	static void Dump(const char* buffer, std::vector<IndexT>& vSize)
	{
		size_t len = sizeof(Type1) * vSize[IndexValue];

		printf("Data Buffer[%u]:\n", IndexValue);
		HexDump(buffer, len, NULL);

		DumpTypeBuffer<Type2, IndexT, IndexValue + 1>::Dump(buffer+len, vSize);
	}
};

template<typename TypeListT, typename HeadT, typename IndexT>
struct MultiBlockHead
{
	HeadT Head;
	IndexT EmptyIndex[TypeListLength<TypeListT>::Length];
	uint64_t Flags;
} __attribute__((packed));

template<typename TypeListT, typename IndexT>
struct MultiBlockHead<TypeListT, void, IndexT>
{
	IndexT EmptyIndex[TypeListLength<TypeListT>::Length];
	uint64_t Flags;
} __attribute__((packed));

template<typename TypeListT, typename HeadT = void, typename IndexT = uint32_t>
class MultiBlockTable
{
public:
	typedef IndexT BlockIndexType;

	static MultiBlockTable<TypeListT, HeadT, IndexT> CreateMultiBlockTable(std::vector<IndexT> vSize)
	{
		size_t size = MultiBlockTable<TypeListT, HeadT, IndexT>::GetBufferSize(vSize);
		if(size > 0)
		{
			char* buffer = (char*)malloc(size);
			memset(buffer, 0, size);
			return MultiBlockTable<TypeListT, HeadT, IndexT>::LoadMultiBlockTable(buffer, size, vSize);
		}
		else
			return MultiBlockTable<TypeListT, HeadT, IndexT>::LoadMultiBlockTable(NULL, 0, vSize);
	}

	static MultiBlockTable<TypeListT, HeadT, IndexT> LoadMultiBlockTable(char* buffer, size_t size, std::vector<IndexT> vSize)
	{
		MultiBlockTable<TypeListT, HeadT, IndexT> mbt;
		if(buffer && MultiBlockTable<TypeListT, HeadT, IndexT>::GetBufferSize(vSize) <= size)
		{
			mbt.m_BlockCount = vSize;
			mbt.m_BlockHead = (MultiBlockHead<TypeListT, HeadT, IndexT>*)buffer;
			mbt.m_BlockBuffer = buffer + sizeof(MultiBlockHead<TypeListT, HeadT, IndexT>);
			if((mbt.m_BlockHead->Flags & BLOCKTABLE_FLAG_INITIALIZE) != BLOCKTABLE_FLAG_INITIALIZE)
			{
				mbt.m_BlockHead->Flags = BLOCKTABLE_FLAG_INITIALIZE;
				for(size_t i=0; i<TypeListLength<TypeListT>::Length; ++i)
					mbt.m_BlockHead->EmptyIndex[i] = 1;
			}
		}
		return mbt;
	}

	template<typename StorageT>
	static inline MultiBlockTable<TypeListT, HeadT, IndexT> LoadMultiBlockTable(StorageT storage, std::vector<IndexT> vSize)
	{
		return LoadMultiBlockTable(storage.GetBuffer(), storage.GetSize(), vSize);
	}

	static size_t GetBufferSize(std::vector<IndexT> vSize)
	{
		if(vSize.size() != TypeListLength<TypeListT>::Length)
			return 0;
		return sizeof(MultiBlockHead<TypeListT, HeadT, IndexT>) + GetTypeBufferSize<TypeListT, IndexT, 0>::Size(vSize);
	}

	void Delete()
	{
		if(m_NeedDelete && m_BlockHead)
		{
			free(m_BlockHead);
			m_BlockHead = NULL;
			m_BlockBuffer = NULL;
		}
	}

	HeadT* GetHead()
	{
		return &m_BlockHead->Head;
	}

	template<typename Type>
	Block<Type, IndexT>* GetBlockBuffer(Block<Type, IndexT>** ppBuffer, IndexT* pCount)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

		*ppBuffer = (Block<Type, IndexT>*)(m_BlockBuffer
								+ GetBufferOffset<TypeListT, Type, IndexT, 0>::Offset(1, m_BlockCount));
		*pCount = m_BlockCount[TypeListIndexOf<TypeListT, Type>::Index];
		return *ppBuffer;
	}

	template<typename Type>
	IndexT AllocateBlock(Type** ppValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

		if(m_BlockHead->EmptyIndex[TypeListIndexOf<TypeListT, Type>::Index] == 0 || 
			m_BlockHead->EmptyIndex[TypeListIndexOf<TypeListT, Type>::Index] > m_BlockCount[TypeListIndexOf<TypeListT, Type>::Index])
		{
			if(ppValue)
				*ppValue = NULL;
			return 0;
		}

		IndexT newBlockId = m_BlockHead->EmptyIndex[TypeListIndexOf<TypeListT, Type>::Index];
		Block<Type, IndexT>* pBlock = (Block<Type, IndexT>*)(m_BlockBuffer
										+ GetBufferOffset<TypeListT, Type, IndexT, 0>::Offset(newBlockId, m_BlockCount));
		if(pBlock->Next == 0)
			++m_BlockHead->EmptyIndex[TypeListIndexOf<TypeListT, Type>::Index];
		else
			m_BlockHead->EmptyIndex[TypeListIndexOf<TypeListT, Type>::Index] = pBlock->Next;

		pBlock->Next = 0;
		pBlock->Flags = BLOCK_FLAG_ACTIVE;
		memset(&pBlock->Value, 0, sizeof(Type));
		if(ppValue)
			*ppValue = &pBlock->Value;
		return newBlockId;
	}

	template<typename Type>
	void ReleaseBlock(Type* pValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

		if(!pValue) return;

		IndexT id = GetBlockID(pValue);
		if(id == 0)
			return;

		Block<Type, IndexT>* pBlock = (Block<Type, IndexT>*)pValue;
		if((pBlock->Flags & BLOCK_FLAG_ACTIVE) != BLOCK_FLAG_ACTIVE)
			return;

		pBlock->Next = m_BlockHead->EmptyIndex[TypeListIndexOf<TypeListT, Type>::Index];
		pBlock->Flags = ~BLOCK_FLAG_ACTIVE;
		m_BlockHead->EmptyIndex[TypeListIndexOf<TypeListT, Type>::Index] = id;
	}

	template<typename Type>
	IndexT GetBlockID(const Type* pValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

		if(!pValue)
			return 0;

		return GetBlockNodeID<TypeListT, Type, IndexT, 0>::ID(m_BlockBuffer, pValue, m_BlockCount);
	}
	
	template<typename Type>
	Type* GetBlock(IndexT id, Type** ppValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

		if(id == 0 || id > m_BlockCount[TypeListIndexOf<TypeListT, Type>::Index])
		{
			if(ppValue)
				*ppValue = NULL;
			return NULL;
		}

		Block<Type, IndexT>* pBlock = (Block<Type, IndexT>*)(m_BlockBuffer
								+ GetBufferOffset<TypeListT, Type, IndexT, 0>::Offset(id, m_BlockCount));
		if((pBlock->Flags & BLOCK_FLAG_ACTIVE) != BLOCK_FLAG_ACTIVE)
		{
			if(ppValue)
				*ppValue = NULL;
			return NULL;
		}

		if(ppValue)
			*ppValue = &pBlock->Value;
		return &pBlock->Value;
	}

	void Dump()
	{
		printf("Head Buffer:\n");
		HexDump((const char*)m_BlockHead, sizeof(MultiBlockHead<TypeListT, HeadT, IndexT>), NULL);

		DumpTypeBuffer<TypeListT, IndexT, 0>::Dump(m_BlockBuffer, m_BlockCount);
	}

	MultiBlockTable() :
		m_NeedDelete(false),
		m_BlockHead(NULL),
		m_BlockBuffer(NULL)
	{
	}

protected:
	bool m_NeedDelete;
	std::vector<IndexT> m_BlockCount;

	MultiBlockHead<TypeListT, HeadT, IndexT>* m_BlockHead;
	char* m_BlockBuffer;
};


#endif // define __BLOCKTABLE_HPP__
