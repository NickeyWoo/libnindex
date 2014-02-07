/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2014.02.12
 *
*--*/
#ifndef __TERNARYTREE_HPP__
#define __TERNARYTREE_HPP__

#include <utility>
#include <vector>

#include "keyutility.hpp"
#include "storage.hpp"
#include "blocktable.hpp"

template<typename KeyT>
struct Length
{
	size_t operator()(const KeyT* pKeyString);
};

template<>
struct Length<wchar_t>
{
	inline size_t operator()(const wchar_t* pKeyString)
	{
		return wcslen(pKeyString);
	}
};

template<>
struct Length<char>
{
	inline size_t operator()(const char* pKeyString)
	{
		return strlen(pKeyString);
	}
};

template<typename IndexT>
struct TernaryNodeHead
{
	uint8_t Color;

	IndexT ParentIndex;
	IndexT LeftIndex;
	IndexT CenterIndex;
	IndexT RightIndex;
} __attribute__((packed));

template<typename KeyT, typename IndexT>
struct TernaryNode
{
	TernaryNodeHead<IndexT> Head;
	KeyT Key;
} __attribute__((packed));

template<typename IndexT>
struct TernaryTreeHead
{
	IndexT RootIndex;
} __attribute__((packed));

template<typename KeyT, typename IndexT = uint32_t>
class TernaryTree
{
public:
	typedef IndexT TernaryTreeIterator;

	static TernaryTree<KeyT, IndexT> CreateTernaryTree(size_t size)
	{
		TernaryTree<KeyT, IndexT> tt;
		tt.m_NodeBlockTable = BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::CreateBlockTable(size);
		return tt;
	}

	static TernaryTree<KeyT, IndexT> LoadTernaryTree(char* buffer, size_t size)
	{
		TernaryTree<KeyT, IndexT> tt;
		tt.m_NodeBlockTable = BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::LoadBlockTable(buffer, size);
		return tt;
	}

	template<typename StorageT>
	static inline TernaryTree<KeyT, IndexT> LoadTernaryTree(StorageT storage)
	{
		return LoadTernaryTree(storage.GetBuffer(), storage.GetSize());
	}

	static inline size_t GetBufferSize(IndexT stringCount, IndexT avgStringLength)
	{
		return BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::GetBufferSize(stringCount * avgStringLength);
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::GetBufferSize(size);
	}

	inline void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	TernaryNode<KeyT, IndexT>* Hash(TernaryNode<KeyT, IndexT>* pLayerRootNode, KeyT key, bool isNew)
	{
		return NULL;
	}

	int Push(const KeyT* pKeyString, size_t size = 0)
	{
		if(!pKeyString)
			return -1;

		if(!size)
		{
			Length<KeyT> len;
			size = len(pKeyString);
		}

		TernaryTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		TernaryNode<KeyT, IndexT>* pRootNode = m_NodeBlockTable[pHead->RootIndex];
		for(size_t i=0; i<size; ++i)
		{
			TernaryNode<KeyT, IndexT>* pLayerNode = Hash(pRootNode, pKeyString[i], true);

			pRootNode = m_NodeBlockTable[pLayerNode->Head.CenterIndex];
		}

		return 0;
	}

	TernaryTreeIterator PrefixSearch(const KeyT* pPrefixKey, size_t size = 0)
	{
		IndexT index;
		return index;
	}

	KeyT* Next(TernaryTreeIterator iter, KeyT* pKeyString, size_t* p)
	{
		return NULL;
	}

	void DumpTree()
	{
	}

	inline void Dump()
	{
		m_NodeBlockTable.Dump();
	}

protected:

	BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT> m_NodeBlockTable;
};


#endif // define __TERNARYTREE_HPP__
