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

#define	BLOCK_FLAG_ACTIVE				1

template<typename ValueT>
struct Block
{
	ValueT Value;

	uint8_t Flags;

	uint32_t Prev;
	uint32_t Next;
} __attribute__((packed));

#define BLOCKTABLE_MAGIC        "BLOCKTAB"
#define MULTIBLOCKTABLE_MAGIC   "MULTBLKT"
#define BLOCKTABLE_VERSION      0x0101

template<typename HeadT>
struct BlockHead
{
    char cMagic[8];
    uint16_t wVersion;

    uint64_t ddwMemSize;
    uint32_t dwHeadSize;

    uint32_t dwTotal;
    uint32_t dwUsed;

	uint32_t EmptyIndex;
	uint32_t ActiveIndex;

    uint32_t dwReserved[4];

	HeadT Head;
} __attribute__((packed));
template<>
struct BlockHead<void>
{
    char cMagic[8];
    uint16_t wVersion;

    uint64_t ddwMemSize;
    uint32_t dwHeadSize;

    uint32_t dwTotal;
    uint32_t dwUsed;

	uint32_t EmptyIndex;
	uint32_t ActiveIndex;

    uint32_t dwReserved[4];
} __attribute__((packed));

struct BlockTableIterator {
    uint32_t Index;
};

template<typename ValueT, typename HeadT = void>
class BlockTable
{
public:
	typedef uint32_t BlockIndexType;

	static BlockTable<ValueT, HeadT> CreateBlockTable(uint32_t size)
	{
        size_t headSize = sizeof(BlockHead<HeadT>);
		size_t bufferSize = headSize + sizeof(Block<ValueT>) * size;

		BlockTable<ValueT, HeadT> bt;
		bt.m_BlockHead = (BlockHead<HeadT>*)malloc(bufferSize);
        if(bt.m_BlockHead == NULL)
            return bt;

		memset(bt.m_BlockHead, 0, bufferSize);

        memcpy(bt.m_BlockHead->cMagic, BLOCKTABLE_MAGIC, 8);
        bt.m_BlockHead->wVersion = BLOCKTABLE_VERSION;
        bt.m_BlockHead->ddwMemSize = bufferSize;
        bt.m_BlockHead->dwHeadSize = headSize;
        bt.m_BlockHead->dwTotal = size;
        bt.m_BlockHead->dwUsed = 0;
		bt.m_BlockHead->EmptyIndex = 1;
		bt.m_BlockHead->ActiveIndex = 0;

		bt.m_BlockBuffer = (Block<ValueT>*)((char*)bt.m_BlockHead + sizeof(BlockHead<HeadT>));
		bt.m_NeedDelete = true;
		return bt;
	}

	static BlockTable<ValueT, HeadT> LoadBlockTable(char* buffer, size_t size)
	{
		BlockTable<ValueT, HeadT> bt;
		bt.m_BlockHead = (BlockHead<HeadT>*)buffer;

        if(memcmp(bt.m_BlockHead->cMagic, "\0\0\0\0\0\0\0\0", 8) == 0)
        {
            memcpy(bt.m_BlockHead->cMagic, BLOCKTABLE_MAGIC, 8);
            bt.m_BlockHead->wVersion = BLOCKTABLE_VERSION;

            bt.m_BlockHead->ddwMemSize = size;
            bt.m_BlockHead->dwHeadSize = sizeof(BlockHead<HeadT>);

            bt.m_BlockHead->dwTotal = (size - sizeof(BlockHead<HeadT>)) / sizeof(Block<ValueT>);
            bt.m_BlockHead->dwUsed = 0;

            bt.m_BlockHead->EmptyIndex = 1;
            bt.m_BlockHead->ActiveIndex = 0;

            bt.m_BlockBuffer = (Block<ValueT>*)(buffer + sizeof(BlockHead<HeadT>));
            return bt;
        }
        else
        {
            if(memcmp(bt.m_BlockHead->cMagic, BLOCKTABLE_MAGIC, 8) != 0 ||
                bt.m_BlockHead->wVersion != BLOCKTABLE_VERSION ||
                bt.m_BlockHead->dwTotal != (size - sizeof(BlockHead<HeadT>)) / sizeof(Block<ValueT>) ||
                bt.m_BlockHead->ddwMemSize != size ||
                bt.m_BlockHead->dwHeadSize != sizeof(BlockHead<HeadT>))
            {
                bt.m_BlockHead = NULL;
                return bt;
            }

            bt.m_BlockBuffer = (Block<ValueT>*)(buffer + sizeof(BlockHead<HeadT>));
            return bt;
        }
	}

	template<typename StorageT>
	static inline BlockTable<ValueT, HeadT> LoadBlockTable(StorageT storage)
	{
		return LoadBlockTable(storage.GetStorageBuffer(), storage.GetSize());
	}

	static inline size_t GetBufferSize(uint32_t count)
	{
		return sizeof(BlockHead<HeadT>) + sizeof(Block<ValueT>) * count;
	}

    bool Success()
    {
        return m_BlockHead != NULL && m_BlockBuffer != NULL;
    }

    float Capacity()
    {
        if(m_BlockHead == NULL || m_BlockBuffer == NULL)
            return 1;
        return (float)m_BlockHead->dwUsed / m_BlockHead->dwTotal;
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

	inline HeadT* GetHead()
	{
        if(!m_BlockHead)
            return NULL;
		return &m_BlockHead->Head;
	}

	inline Block<ValueT>* GetBlockBuffer(Block<ValueT>** ppBuffer, uint32_t* pCount)
	{
		*ppBuffer = m_BlockBuffer;
		*pCount = m_BlockHead->dwTotal;
		return *ppBuffer;
	}

    BlockTableIterator Begin()
    {
        BlockTableIterator iter;
        iter.Index = m_BlockHead->ActiveIndex;
        return iter;
    }

    ValueT* Next(BlockTableIterator* pstIterator)
    {
        if(pstIterator && pstIterator->Index > 0)
        {
            Block<ValueT>* pBlock = &m_BlockBuffer[pstIterator->Index - 1];
            pstIterator->Index = pBlock->Next;
            return &pBlock->Value;
        }
        return NULL;
    }

	uint32_t AllocateBlock()
	{
		if(m_BlockHead == NULL || m_BlockBuffer == NULL || 
            m_BlockHead->EmptyIndex == 0 || m_BlockHead->EmptyIndex > m_BlockHead->dwTotal)
			return 0;

		uint32_t newBlockId = m_BlockHead->EmptyIndex;
		Block<ValueT>* pBlock = &m_BlockBuffer[newBlockId - 1];
		if(pBlock->Next == 0)
			++m_BlockHead->EmptyIndex;
		else
			m_BlockHead->EmptyIndex = pBlock->Next;

        pBlock->Prev = 0;
		pBlock->Next = m_BlockHead->ActiveIndex;
		pBlock->Flags = BLOCK_FLAG_ACTIVE;
		memset(&pBlock->Value, 0, sizeof(ValueT));

        if(m_BlockHead->ActiveIndex > 0)
        {
            Block<ValueT>* pNextActiveBlock = &m_BlockBuffer[m_BlockHead->ActiveIndex - 1];
            pNextActiveBlock->Prev = newBlockId;
        }
        m_BlockHead->ActiveIndex = newBlockId;

        ++m_BlockHead->dwUsed;
		return newBlockId;
	}

	void ReleaseBlock(uint32_t id)
	{
		if(m_BlockHead == NULL || m_BlockBuffer == NULL || 
            id == 0 || id > m_BlockHead->dwTotal)
			return;

		Block<ValueT>* pBlock = &m_BlockBuffer[id - 1];
		if((pBlock->Flags & BLOCK_FLAG_ACTIVE) != BLOCK_FLAG_ACTIVE)
			return;

        if(pBlock->Next > 0)
        {
            Block<ValueT>* pNextActiveBlock = &m_BlockBuffer[pBlock->Next - 1];
            pNextActiveBlock->Prev = pBlock->Prev;
        }

        if(pBlock->Prev > 0)
        {
            Block<ValueT>* pPrevActiveBlock = &m_BlockBuffer[pBlock->Prev - 1];
            pPrevActiveBlock->Next = pBlock->Next;
        }

        if(id == m_BlockHead->ActiveIndex)
            m_BlockHead->ActiveIndex = pBlock->Next;

        pBlock->Prev = 0;
		pBlock->Next = m_BlockHead->EmptyIndex;
		pBlock->Flags = pBlock->Flags & ~BLOCK_FLAG_ACTIVE;

		m_BlockHead->EmptyIndex = id;

        --m_BlockHead->dwUsed;
	}

	inline uint32_t GetBlockID(ValueT* pVal)
	{
		if(m_BlockHead == NULL || m_BlockBuffer == NULL || pVal == NULL)
			return 0;
		else
			return ((char*)pVal - (char*)m_BlockBuffer) / sizeof(Block<ValueT>) + 1;
	}

	inline ValueT* GetBlock(uint32_t id)
	{
		if(m_BlockHead == NULL || m_BlockBuffer == NULL || id == 0 || id > m_BlockHead->dwTotal)
			return NULL;
		return &(m_BlockBuffer[id - 1].Value);
	}

	inline ValueT* operator[](uint32_t id)
	{
		return GetBlock(id);
	}

	void Dump()
	{
		printf("Head Buffer:\n");
		HexDump((const char*)m_BlockHead, sizeof(BlockHead<HeadT>), NULL);
		printf("Data Buffer:\n");
		HexDump((const char*)m_BlockBuffer, sizeof(Block<ValueT>) * m_BlockHead->dwTotal, NULL);
	}

	BlockTable() :
		m_NeedDelete(false),
		m_BlockHead(NULL),
		m_BlockBuffer(NULL)
	{
	}

protected:
	bool m_NeedDelete;

	BlockHead<HeadT>* m_BlockHead;
	Block<ValueT>* m_BlockBuffer;
};


/////////////////////////////////////////////////////////////////////////////////////////////////
// MultiBlockTable

template<typename TypeListT, uint32_t IndexValue>
struct GetTypeBufferSize
{
	static size_t Size(std::vector<uint32_t>& vSize);
};
template<uint32_t IndexValue>
struct GetTypeBufferSize<NullType, IndexValue>
{
	static inline size_t Size(std::vector<uint32_t>& vSize)
	{
		return 0;
	}
};
template<typename Type1, typename Type2, uint32_t IndexValue>
struct GetTypeBufferSize<TypeList<Type1, Type2>, IndexValue>
{
	static inline size_t Size(std::vector<uint32_t>& vSize)
	{
		return (sizeof(Block<Type1>) * vSize[IndexValue])
					+ GetTypeBufferSize<Type2, IndexValue + 1>::Size(vSize);
	}
};

template<typename TypeListT, typename TypeT, uint32_t IndexValue>
struct GetBufferOffset
{
	static size_t Offset(uint32_t index, uint32_t* pSizeBuffer);
};
template<typename TypeT, uint32_t IndexValue>
struct GetBufferOffset<NullType, TypeT, IndexValue>
{
	static inline size_t Offset(uint32_t index, uint32_t* pSizeBuffer)
	{
		return 0;
	}
};
template<typename Type2, typename TypeT, uint32_t IndexValue>
struct GetBufferOffset<TypeList<TypeT, Type2>, TypeT, IndexValue>
{
	static inline size_t Offset(uint32_t index, uint32_t* pSizeBuffer)
	{
		return sizeof(Block<TypeT>) * (index - 1);
	}
};
template<typename Type1, typename Type2, typename TypeT, uint32_t IndexValue>
struct GetBufferOffset<TypeList<Type1, Type2>, TypeT, IndexValue>
{
	static inline size_t Offset(uint32_t index, uint32_t* pSizeBuffer)
	{
		return (sizeof(Block<Type1>) * pSizeBuffer[IndexValue])
					+ GetBufferOffset<Type2, TypeT, IndexValue + 1>::Offset(index, pSizeBuffer);
	}
};

template<typename TypeListT, typename TypeT, uint32_t IndexValue>
struct GetBlockNodeID
{
	static uint32_t ID(char* buffer, const TypeT* pValue, uint32_t* pSizeBuffer);
};
template<typename TypeT, uint32_t IndexValue>
struct GetBlockNodeID<NullType, TypeT, IndexValue>
{
	static inline uint32_t ID(char* buffer, const TypeT* pValue, uint32_t* pSizeBuffer)
	{
		return 0;
	}
};
template<typename Type2, typename TypeT, uint32_t IndexValue>
struct GetBlockNodeID<TypeList<TypeT, Type2>, TypeT, IndexValue>
{
	static inline uint32_t ID(char* buffer, const TypeT* pValue, uint32_t* pSizeBuffer)
	{
		return ((char*)pValue - buffer) / sizeof(Block<TypeT>) + 1;
	}
};
template<typename Type1, typename Type2, typename TypeT, uint32_t IndexValue>
struct GetBlockNodeID<TypeList<Type1, Type2>, TypeT, IndexValue>
{
	static inline uint32_t ID(char* buffer, const TypeT* pValue, uint32_t* pSizeBuffer)
	{
		return GetBlockNodeID<Type2, TypeT, IndexValue + 1>::ID((buffer + 
                    sizeof(Block<Type1>) * pSizeBuffer[IndexValue]), pValue, pSizeBuffer);
	}
};

template<typename TypeListT, uint32_t IndexValue>
struct DumpTypeBuffer
{
	static void Dump(const char* buffer, uint32_t* pSizeBuffer);
};
template<uint32_t IndexValue>
struct DumpTypeBuffer<NullType, IndexValue>
{
	static void Dump(const char* buffer, uint32_t* pSizeBuffer)
	{
	}
};
template<typename Type1, typename Type2, uint32_t IndexValue>
struct DumpTypeBuffer<TypeList<Type1, Type2>, IndexValue>
{
	static void Dump(const char* buffer, uint32_t* pSizeBuffer)
	{
		size_t len = sizeof(Type1) * pSizeBuffer[IndexValue];

		printf("Data Buffer[%u]:\n", IndexValue);
		HexDump(buffer, len, NULL);

		DumpTypeBuffer<Type2, IndexValue + 1>::Dump(buffer+len, pSizeBuffer);
	}
};

template<typename TypeListT, typename HeadT>
struct MultiBlockHead
{
    char cMagic[8];
    uint16_t wVersion;

    uint64_t ddwMemSize;
    uint32_t dwHeadSize;

    uint8_t cMulti;
    uint32_t Total[TypeListLength<TypeListT>::Length];
    uint32_t Used[TypeListLength<TypeListT>::Length];
	uint32_t EmptyIndex[TypeListLength<TypeListT>::Length];

    uint32_t dwReserved[4];

	HeadT Head;
} __attribute__((packed));
template<typename TypeListT>
struct MultiBlockHead<TypeListT, void>
{
    char cMagic[8];
    uint16_t wVersion;

    uint64_t ddwMemSize;
    uint32_t dwHeadSize;

    uint8_t cMulti;
    uint32_t Total[TypeListLength<TypeListT>::Length];
    uint32_t Used[TypeListLength<TypeListT>::Length];
	uint32_t EmptyIndex[TypeListLength<TypeListT>::Length];

    uint32_t dwReserved[4];
} __attribute__((packed));

template<typename TypeListT, typename HeadT = void>
class MultiBlockTable
{
public:
	typedef uint32_t BlockIndexType;

	static MultiBlockTable<TypeListT, HeadT> CreateMultiBlockTable(std::vector<uint32_t> vSize)
	{
		size_t size = MultiBlockTable<TypeListT, HeadT>::GetBufferSize(vSize);
        if(size == 0)
            return MultiBlockTable<TypeListT, HeadT>();

        char* buffer = (char*)malloc(size);
        if(buffer == NULL)
            return MultiBlockTable<TypeListT, HeadT>();

        memset(buffer, 0, size);
		MultiBlockTable<TypeListT, HeadT> mbt = MultiBlockTable<TypeListT, HeadT>::
                                                    LoadMultiBlockTable(buffer, size, vSize);
        mbt.m_NeedDelete = true;
        return mbt;
	}

	static MultiBlockTable<TypeListT, HeadT> LoadMultiBlockTable(char* buffer, size_t size, 
                                                                 std::vector<uint32_t> vSize)
	{
		MultiBlockTable<TypeListT, HeadT> mbt;
		if(buffer && TypeListLength<TypeListT>::Length == vSize.size() &&
            MultiBlockTable<TypeListT, HeadT>::GetBufferSize(vSize) <= size)
		{
			mbt.m_BlockHead = (MultiBlockHead<TypeListT, HeadT>*)buffer;
			mbt.m_BlockBuffer = buffer + sizeof(MultiBlockHead<TypeListT, HeadT>);

            if(memcmp(mbt.m_BlockHead->cMagic, "\0\0\0\0\0\0\0\0", 8) == 0)
			{
                memcpy(mbt.m_BlockHead->cMagic, MULTIBLOCKTABLE_MAGIC, 8);
                mbt.m_BlockHead->wVersion = BLOCKTABLE_VERSION;

                mbt.m_BlockHead->ddwMemSize = size;
                mbt.m_BlockHead->dwHeadSize = sizeof(MultiBlockHead<TypeListT, HeadT>);
                mbt.m_BlockHead->cMulti = TypeListLength<TypeListT>::Length;

				for(size_t i=0; i<TypeListLength<TypeListT>::Length; ++i)
                {
					mbt.m_BlockHead->Total[i] = vSize.at(i);
					mbt.m_BlockHead->Used[i] = 0;
					mbt.m_BlockHead->EmptyIndex[i] = 1;
                }
			}
            else
            {
                if(memcmp(mbt.m_BlockHead->cMagic, MULTIBLOCKTABLE_MAGIC, 8) != 0 ||
                    mbt.m_BlockHead->wVersion != BLOCKTABLE_VERSION ||
                    mbt.m_BlockHead->dwHeadSize != sizeof(MultiBlockHead<TypeListT, HeadT>) ||
                    mbt.m_BlockHead->ddwMemSize != size ||
                    mbt.m_BlockHead->cMulti != vSize.size())
                {
                    mbt.m_BlockHead = NULL;
                    mbt.m_BlockBuffer = NULL;
                    return mbt;
                }

				for(size_t i=0; i<TypeListLength<TypeListT>::Length; ++i)
                {
                    if(mbt.m_BlockHead->Total[i] != vSize.at(i))
                    {
                        mbt.m_BlockHead = NULL;
                        mbt.m_BlockBuffer = NULL;
                        return mbt;
                    }
                }
            }
		}
		return mbt;
	}

	template<typename StorageT>
	static inline MultiBlockTable<TypeListT, HeadT> LoadMultiBlockTable(StorageT storage, 
                                                                        std::vector<uint32_t> vSize)
	{
		return LoadMultiBlockTable(storage.GetBuffer(), storage.GetSize(), vSize);
	}

	static size_t GetBufferSize(std::vector<uint32_t> vSize)
	{
		if(vSize.size() != TypeListLength<TypeListT>::Length)
			return 0;
		return sizeof(MultiBlockHead<TypeListT, HeadT>) + GetTypeBufferSize<TypeListT, 0>::Size(vSize);
	}

    inline bool Success()
    {
        return m_BlockHead != NULL && m_BlockBuffer != NULL;
    }

    float Capacity()
    {
        if(m_BlockHead == NULL || m_BlockBuffer == NULL)
            return 1;

        float fMax = 0;
        for(int i=0; i<TypeListLength<TypeListT>::Length; ++i)
        {
            float fCapacity = (float)m_BlockHead->Used[i] / m_BlockHead->Total[i];
            if(fCapacity > fMax)
                fMax = fCapacity;
        }
        return fMax;
    }

	void Delete()
	{
		if(m_NeedDelete && m_BlockHead)
			free(m_BlockHead);

        m_BlockHead = NULL;
        m_BlockBuffer = NULL;
	}

	HeadT* GetHead()
	{
        if(m_BlockHead == NULL || m_BlockBuffer == NULL)
            return NULL;
		return &m_BlockHead->Head;
	}

	template<typename Type>
	Block<Type>* GetBlockBuffer(Block<Type>** ppBuffer, uint32_t* pCount)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

        if(m_BlockHead == NULL || m_BlockBuffer == NULL)
            return NULL;

		*ppBuffer = (Block<Type>*)(m_BlockBuffer + 
                                   GetBufferOffset<TypeListT, Type, 0>::Offset(1, m_BlockHead->Total));
		*pCount = m_BlockHead->Total[TypeListIndexOf<TypeListT, Type>::Index];
		return *ppBuffer;
	}

	template<typename Type>
	uint32_t AllocateBlock(Type** ppValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

        if(m_BlockHead == NULL || m_BlockBuffer == NULL)
            return 0;

        uint32_t idx = TypeListIndexOf<TypeListT, Type>::Index;
		if(m_BlockHead->EmptyIndex[idx] == 0 || 
			m_BlockHead->EmptyIndex[idx] > m_BlockHead->Total[idx])
		{
			if(ppValue)
				*ppValue = NULL;
			return 0;
		}

		uint32_t newBlockId = m_BlockHead->EmptyIndex[idx];
		Block<Type>* pBlock = (Block<Type>*)(m_BlockBuffer
                                + GetBufferOffset<TypeListT, Type, 0>::Offset(newBlockId, m_BlockHead->Total));
		if(pBlock->Next == 0)
			++m_BlockHead->EmptyIndex[idx];
		else
			m_BlockHead->EmptyIndex[idx] = pBlock->Next;

		pBlock->Next = 0;
		pBlock->Flags = BLOCK_FLAG_ACTIVE;
		memset(&pBlock->Value, 0, sizeof(Type));
		if(ppValue)
			*ppValue = &pBlock->Value;

        ++m_BlockHead->Used[idx];
		return newBlockId;
	}

	template<typename Type>
	void ReleaseBlock(Type* pValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

        if(m_BlockHead == NULL || m_BlockBuffer == NULL || pValue == NULL)
            return;

		uint32_t id = GetBlockID(pValue);
		if(id == 0)
			return;

		Block<Type>* pBlock = (Block<Type>*)pValue;
		if((pBlock->Flags & BLOCK_FLAG_ACTIVE) != BLOCK_FLAG_ACTIVE)
			return;

        uint32_t idx = TypeListIndexOf<TypeListT, Type>::Index;
		pBlock->Next = m_BlockHead->EmptyIndex[idx];
		pBlock->Flags = pBlock->Flags & ~BLOCK_FLAG_ACTIVE;
		m_BlockHead->EmptyIndex[idx] = id;
        --m_BlockHead->Used[idx];
	}

	template<typename Type>
	uint32_t GetBlockID(const Type* pValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

        if(m_BlockHead == NULL || m_BlockBuffer == NULL || pValue == NULL)
			return 0;

		return GetBlockNodeID<TypeListT, Type, 0>::ID(m_BlockBuffer, pValue, m_BlockHead->Total);
	}
	
	template<typename Type>
	Type* GetBlock(uint32_t id, Type** ppValue)
	{
		BOOST_STATIC_ASSERT((TypeListIndexOf<TypeListT, Type>::Index >= 0));

		if(m_BlockHead == NULL || m_BlockBuffer == NULL ||
            id == 0 || id > m_BlockHead->Total[TypeListIndexOf<TypeListT, Type>::Index])
		{
			if(ppValue)
				*ppValue = NULL;
			return NULL;
		}

		Block<Type>* pBlock = (Block<Type>*)(m_BlockBuffer
								+ GetBufferOffset<TypeListT, Type, 0>::Offset(id, m_BlockHead->Total));
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
		HexDump((const char*)m_BlockHead, sizeof(MultiBlockHead<TypeListT, HeadT>), NULL);

		DumpTypeBuffer<TypeListT, 0>::Dump(m_BlockBuffer, m_BlockHead->Total);
	}

	MultiBlockTable() :
		m_NeedDelete(false),
		m_BlockHead(NULL),
		m_BlockBuffer(NULL)
	{
	}

protected:
	bool m_NeedDelete;

	MultiBlockHead<TypeListT, HeadT>* m_BlockHead;
	char* m_BlockBuffer;
};


#endif // define __BLOCKTABLE_HPP__
