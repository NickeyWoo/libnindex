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
#include "utility.hpp"
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

template<typename ValueT, typename KeyT = char, typename IndexT = int32_t>
class TernaryTree
{
public:
	typedef IndexT TernaryTreeIterator;

	static TernaryTree<ValueT, KeyT, IndexT> CreateTernaryTree(IndexT stringCount, IndexT avgStringLength)
	{
		std::vector<IndexT> vec;
		vec.push_back(stringCount*avgStringLength);
		vec.push_back(stringCount);

		TernaryTree<ValueT, KeyT, IndexT> tt;
		tt.m_NodeBlockTable = MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), 
								TernaryTreeHead<IndexT>, IndexT>::CreateMultiBlockTable(vec);
		return tt;
	}

	static TernaryTree<ValueT, KeyT, IndexT> LoadTernaryTree(char* buffer, size_t size, IndexT stringCount, IndexT avgStringLength)
	{
		std::vector<IndexT> vec;
		vec.push_back(stringCount*avgStringLength);
		vec.push_back(stringCount);

		TernaryTree<ValueT, KeyT, IndexT> tt;
		tt.m_NodeBlockTable = MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), 
								TernaryTreeHead<IndexT>, IndexT>::LoadMultiBlockTable(buffer, size, vec);
		return tt;
	}

	template<typename StorageT>
	static inline TernaryTree<ValueT, KeyT, IndexT> LoadTernaryTree(StorageT storage, IndexT stringCount, IndexT avgStringLength)
	{
		return LoadTernaryTree(storage.GetBuffer(), storage.GetSize(), stringCount, avgStringLength);
	}

	static inline size_t GetBufferSize(IndexT stringCount, IndexT avgStringLength)
	{
		std::vector<IndexT> vec;
		vec.push_back(stringCount*avgStringLength);
		vec.push_back(stringCount);

		return MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), 
					TernaryTreeHead<IndexT>, IndexT>::GetBufferSize(vec);
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
		TernaryNode<KeyT, IndexT>* pRootNode = NULL;
		m_NodeBlockTable.GetBlock(pHead->RootIndex, &pRootNode);
		for(size_t i=0; i<size; ++i)
		{
			TernaryNode<KeyT, IndexT>* pLayerNode = Hash(pRootNode, pKeyString[i], true);

			m_NodeBlockTable.GetBlock(pLayerNode->Head.CenterIndex, &pRootNode);
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

	typedef TernaryNode<KeyT, IndexT> TreeNodeType;

	MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), TernaryTreeHead<IndexT>, IndexT> m_NodeBlockTable;

};


#endif // define __TERNARYTREE_HPP__
