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

#define NODECOLOR_RED			0
#define NODECOLOR_BLACK			1
#define NODECOLOR_LEAF			2

template<typename IndexT>
struct TernaryNodeHead
{
	uint8_t Color;

	IndexT ParentIndex;
	IndexT LeftIndex;
	IndexT RightIndex;

	IndexT CenterIndex;
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

template<typename ValueT, typename KeyT = char, typename IndexT = uint32_t>
class TernaryTree
{
public:
	typedef IndexT TernaryTreeIterator;
	typedef TernaryNode<KeyT, IndexT> TreeNodeType;

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

	void Clear(const KeyT* pKeyString, size_t size = 0)
	{
		if(!pKeyString)
			return;

		if(!size)
		{
			Length<KeyT> len;
			size = len(pKeyString);
		}

		TernaryTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();

	}

	ValueT* Hash(const KeyT* pKeyString, bool isNew = false, size_t size = 0)
	{
		if(!pKeyString)
			return NULL;

		if(!size)
		{
			Length<KeyT> len;
			size = len(pKeyString);
		}

		IndexT* pEmptyIdx;
		IndexT ParentIdx;

		TernaryTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		IndexT* pRootNodeIdx = &pHead->RootIndex;
		for(size_t i=0; i<size; ++i)
		{
			TreeNodeType* pRootNode = FindKey(pKeyString[i], pRootNodeIdx, &pEmptyIdx, &ParentIdx);
			if(pRootNode == NULL)
			{
				if(isNew)
					return InsertString(&pKeyString[i], size - i, pEmptyIdx, ParentIdx, pRootNodeIdx);
				return NULL;
			}

			pRootNodeIdx = &pRootNode->Head.CenterIndex;
		}

		TreeNodeType* pLeafNode = FindLeaf(pRootNodeIdx, &pEmptyIdx, &ParentIdx);
		if(pLeafNode == NULL)
		{
			if(isNew)
				return InsertLeaf(pEmptyIdx, ParentIdx, pRootNodeIdx);
			return NULL;
		}

		ValueT* pValue = NULL;
		m_NodeBlockTable.GetBlock(pLeafNode->Head.CenterIndex, &pValue);
		return pValue;
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

	void InsertFixup(TreeNodeType* pNode, IndexT* pRootNodeIdx)
	{
	}

	ValueT* InsertString(const KeyT* pKeyString, size_t size, IndexT* pEmptyIdx, IndexT ParentIdx, IndexT* pRootNodeIdx)
	{
		ValueT* pValue = NULL;
		IndexT ValueIdx = m_NodeBlockTable.AllocateBlock(&pValue);
		if(!pValue)
			return NULL;

		std::vector<TreeNodeType*> vNodePtr;

		TreeNodeType* pNode = NULL;
		IndexT nodeIdx = m_NodeBlockTable.AllocateBlock(&pNode);
		if(!pNode)
		{
			m_NodeBlockTable.ReleaseBlock(pValue);
			return NULL;
		}
		pNode->Head.Color = NODECOLOR_LEAF|NODECOLOR_RED;
		pNode->Head.CenterIndex = ValueIdx;

		vNodePtr.push_back(pNode);

		bool error = false;
		for(size_t i=size-1; i>=0; ++i)
		{
			TreeNodeType* pTempNode = NULL;
			pNode->Head.ParentIndex = m_NodeBlockTable.AllocateBlock(&pTempNode);
			if(pTempNode == NULL)
			{
				error = true;
				break;
			}

			pTempNode->Head.CenterIndex = nodeIdx;
			pTempNode->Head.Color = NODECOLOR_RED;
			pTempNode->Key = pKeyString[i];

			pNode->Head.Color = (pNode->Head.Color & NODECOLOR_LEAF) | NODECOLOR_BLACK;

			nodeIdx = pNode->Head.ParentIndex;
			pNode = pTempNode;

			vNodePtr.push_back(pNode);
		}
		if(error)
		{
			m_NodeBlockTable.ReleaseBlock(pValue);
			for(typename std::vector<TreeNodeType*>::iterator iter = vNodePtr.begin();
				iter != vNodePtr.end();
				++iter)
			{
				m_NodeBlockTable.ReleaseBlock(*iter);
			}
			return NULL;
		}

		pNode->Head.ParentIndex = ParentIdx;
		*pEmptyIdx = nodeIdx;

		InsertFixup(pNode, pRootNodeIdx);
		return pValue;
	}

	ValueT* InsertLeaf(IndexT* pEmptyIdx, IndexT ParentIdx, IndexT* pRootNodeIdx)
	{
		if(!pEmptyIdx)
			return NULL;

		TreeNodeType* pLeafNode = NULL;
		*pEmptyIdx = m_NodeBlockTable.AllocateBlock(&pLeafNode);
		if(!pLeafNode)
			return NULL;

		ValueT* pValue = NULL;
		pLeafNode->Head.CenterIndex = m_NodeBlockTable.AllocateBlock(&pValue);
		if(!pValue)
		{
			*pEmptyIdx = 0;
			m_NodeBlockTable.ReleaseBlock(pLeafNode);
			return NULL;
		}

		pLeafNode->Head.Color = NODECOLOR_LEAF|NODECOLOR_RED;
		pLeafNode->Head.ParentIndex = ParentIdx;

		InsertFixup(pLeafNode, pRootNodeIdx);
		return pValue;
	}

	inline TreeNodeType* TreeNodeMinimum(TreeNodeType* pNode)
	{
		TreeNodeType* pLeftNode = NULL;
		while(pNode && m_NodeBlockTable.GetBlock(pNode->Head.LeftIndex, &pLeftNode))
			pNode = pLeftNode;
		return pNode;
	}

	TreeNodeType* FindLeaf(IndexT* pRootNodeIdx, IndexT** ppEmptyIdx, IndexT* pParentIdx)
	{
		if(!pRootNodeIdx || !ppEmptyIdx || !pParentIdx)
			return NULL;

		*ppEmptyIdx = pRootNodeIdx;
		*pParentIdx = 0;

		TreeNodeType* pNode = NULL;
		m_NodeBlockTable.GetBlock(*pRootNodeIdx, &pNode);
		if(!pNode)
			return NULL;

		pNode = TreeNodeMinimum(pNode);
		if((pNode->Head.Color & NODECOLOR_LEAF) == NODECOLOR_LEAF)
			return pNode;

		*ppEmptyIdx = &pNode->Head.LeftIndex;
		*pParentIdx = m_NodeBlockTable.GetBlockID(pNode);
		return NULL;
	}

	TreeNodeType* FindKey(const KeyT key, IndexT* pRootNodeIdx, IndexT** ppEmptyIdx, IndexT* pParentIdx)
	{
		if(!pRootNodeIdx || !ppEmptyIdx || !pParentIdx)
			return NULL;

		*ppEmptyIdx = pRootNodeIdx;
		*pParentIdx = 0;

		TreeNodeType* pNode = NULL;
		IndexT nodeIdx = *pRootNodeIdx;
		while(m_NodeBlockTable.GetBlock(nodeIdx, &pNode) != NULL)
		{
			int result = -1;
			if((pNode->Head.Color & NODECOLOR_LEAF) != NODECOLOR_LEAF)
				result = KeyCompare<KeyT>::Compare(pNode->Key, key);
			if(result == 0)
				return pNode;
			else if(result > 0)
			{
				*ppEmptyIdx = &pNode->Head.LeftIndex;
				*pParentIdx = nodeIdx;
				nodeIdx = pNode->Head.LeftIndex;
			}
			else
			{
				*ppEmptyIdx = &pNode->Head.RightIndex;
				*pParentIdx = nodeIdx;
				nodeIdx = pNode->Head.RightIndex;
			}
		}
		return NULL;
	}

	MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), TernaryTreeHead<IndexT>, IndexT> m_NodeBlockTable;

};


#endif // define __TERNARYTREE_HPP__
