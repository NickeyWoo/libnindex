/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.24
 *
*--*/
#ifndef __BLOCKTABLE_HPP__
#define __BLOCKTABLE_HPP__

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

#endif // define __BLOCKTABLE_HPP__
