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

template<typename IndexT>
struct TernaryTreeIteratorImpl
{
	std::vector<IndexT> LayerIndexVector;
	std::vector<IndexT> PrefixNodeIndexVector;
};

template<typename ValueT, typename KeyT = char, typename IndexT = uint32_t>
class TernaryTree
{
public:
	typedef TernaryTreeIteratorImpl<IndexT> TernaryTreeIterator;

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

	inline ValueT* Hash(const KeyT* pKeyString, bool isNew = false)
	{
		Length<KeyT> len;
		return Hash(pKeyString, len(pKeyString), isNew);
	}

	ValueT* Hash(const KeyT* pKeyString, size_t size, bool isNew = false)
	{
		if(!pKeyString)
			return NULL;

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

	inline TernaryTreeIterator PrefixSearch(const KeyT* pPrefixKey)
	{
		Length<KeyT> len;
		return PrefixSearch(pPrefixKey, len(pPrefixKey));
	}

	TernaryTreeIterator PrefixSearch(const KeyT* pPrefixKey, size_t size)
	{
		TernaryTreeIterator iter;
		if(!pPrefixKey)
			return iter;

		TernaryTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		IndexT* pRootNodeIdx = &pHead->RootIndex;
		for(size_t i=0; i<size; ++i)
		{
			IndexT* pEmptyIdx;
			IndexT ParentIdx;

			TreeNodeType* pRootNode = FindKey(pPrefixKey[i], pRootNodeIdx, &pEmptyIdx, &ParentIdx);
			if(pRootNode == NULL)
			{
				iter.PrefixNodeIndexVector.clear();
				return iter;
			}

			iter.PrefixNodeIndexVector.push_back(m_NodeBlockTable.GetBlockID(pRootNode));

			pRootNodeIdx = &pRootNode->Head.CenterIndex;
		}

		MinimumString(&iter, *pRootNodeIdx);
		return iter;
	}

	ValueT* Next(TernaryTreeIterator* pIter, KeyT* pKeyString = NULL, size_t size = 0, size_t* pwsize = NULL)
	{
		if(!pIter || pIter->LayerIndexVector.empty())
			return NULL;

		ValueT* pValue = NULL;
		TreeNodeType* pLeafNode = NULL;
		if(m_NodeBlockTable.GetBlock(pIter->LayerIndexVector.back(), &pLeafNode) && IsLeaf(pLeafNode))
			m_NodeBlockTable.GetBlock(pLeafNode->Head.CenterIndex, &pValue);

		if(!pValue)
			return NULL;

		if(pKeyString)
		{
			size_t ret = pIter->PrefixNodeIndexVector.size() + pIter->LayerIndexVector.size();
			if(size < ret)
				ret = size;

			size_t i = 0;
			for(; i<ret && i<pIter->PrefixNodeIndexVector.size(); ++i)
			{
				IndexT idx = pIter->PrefixNodeIndexVector[i];

				TreeNodeType* pNode = NULL;
				m_NodeBlockTable.GetBlock(idx, &pNode);

				pKeyString[i] = pNode->Key;
			}

			for(size_t j=0; i<ret && j<pIter->LayerIndexVector.size(); ++j, ++i)
			{
				IndexT idx = pIter->LayerIndexVector[j];

				TreeNodeType* pNode = NULL;
				m_NodeBlockTable.GetBlock(idx, &pNode);

				pKeyString[i] = pNode->Key;
			}

			if(pwsize)
				*pwsize = ret;
		}

		MoveNext(pIter);
		return pValue;
	}

	void DumpTree()
	{
		TernaryTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();

		TreeNodeType* pNode = NULL;
		m_NodeBlockTable.GetBlock(pHead->RootIndex, &pNode);

		std::vector<TreeNodeType*> vPtr;
		PrintNode(pNode, vPtr);
	}

	inline void Dump()
	{
		m_NodeBlockTable.Dump();
	}

protected:
	typedef TernaryNode<KeyT, IndexT> TreeNodeType;

	inline bool IsLeaf(TreeNodeType* pNode)
	{
		return (pNode->Head.Color & NODECOLOR_LEAF) == NODECOLOR_LEAF;
	}

	inline bool IsRed(TreeNodeType* pNode)
	{
		return (pNode->Head.Color & NODECOLOR_BLACK) != NODECOLOR_BLACK;
	}

	inline void SetNodeColorRed(TreeNodeType* pNode)
	{
		pNode->Head.Color = (pNode->Head.Color & NODECOLOR_LEAF);
	}

	inline void SetNodeColorBlack(TreeNodeType* pNode)
	{
		pNode->Head.Color = (pNode->Head.Color & NODECOLOR_LEAF) | NODECOLOR_BLACK;
	}

	void MinimumString(TernaryTreeIterator* pIter, IndexT RootIndex)
	{
		TreeNodeType* pRootNode = NULL;
		while(m_NodeBlockTable.GetBlock(RootIndex, &pRootNode))
		{
			pRootNode = TreeNodeMinimum(pRootNode);
			pIter->LayerIndexVector.push_back(m_NodeBlockTable.GetBlockID(pRootNode));

			if(IsLeaf(pRootNode))
				break;

			RootIndex = pRootNode->Head.CenterIndex;
		}
	}

	void MoveNext(TernaryTreeIterator* pIter)
	{
		while(!pIter->LayerIndexVector.empty())
		{
			IndexT cur = pIter->LayerIndexVector.back();
			pIter->LayerIndexVector.pop_back();

			TreeNodeType* pNode = NULL;
			if(!m_NodeBlockTable.GetBlock(cur, &pNode))
				continue;

			TreeNodeType* pRightNode = NULL;
			if(m_NodeBlockTable.GetBlock(pNode->Head.RightIndex, &pRightNode))
			{
				TreeNodeType* pMiniNode = TreeNodeMinimum(pRightNode);
				pIter->LayerIndexVector.push_back(m_NodeBlockTable.GetBlockID(pMiniNode));
				MinimumString(pIter, pMiniNode->Head.CenterIndex);
				return;
			}
			else
			{
				TreeNodeType* pParentNode = NULL;
				while(m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode) && pParentNode->Head.RightIndex == cur)
				{
					cur = pNode->Head.ParentIndex;
					pNode = pParentNode;
				}

				if(!pParentNode)
					continue;
				
				pIter->LayerIndexVector.push_back(pNode->Head.ParentIndex);
				MinimumString(pIter, pParentNode->Head.CenterIndex);
				return;
			}
		}
	}

	void TreeNodeRotateLeft(TreeNodeType* pNode, IndexT* pRootNodeIdx)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

		TreeNodeType* pRightNode = NULL;
		if(!m_NodeBlockTable.GetBlock(pNode->Head.RightIndex, &pRightNode))
			return;
		pRightNode->Head.ParentIndex = pNode->Head.ParentIndex;

		TreeNodeType* pParentNode = NULL;
		if(!m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode))
			*pRootNodeIdx = pNode->Head.RightIndex;
		else
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.RightIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.RightIndex;
		}

		pNode->Head.ParentIndex = pNode->Head.RightIndex;
		pNode->Head.RightIndex = pRightNode->Head.LeftIndex;

		TreeNodeType* pRightLeftNode = NULL;
		if(m_NodeBlockTable.GetBlock(pRightNode->Head.LeftIndex, &pRightLeftNode))
			pRightLeftNode->Head.ParentIndex = nodeIndex;

		pRightNode->Head.LeftIndex = nodeIndex;
	}

	void TreeNodeRotateRight(TreeNodeType* pNode, IndexT* pRootNodeIdx)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

		TreeNodeType* pLeftNode = NULL;
		if(!m_NodeBlockTable.GetBlock(pNode->Head.LeftIndex, &pLeftNode))
			return;
		pLeftNode->Head.ParentIndex = pNode->Head.ParentIndex;

		TreeNodeType* pParentNode = NULL;
		if(!m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode))
			*pRootNodeIdx = pNode->Head.LeftIndex;
		else
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.LeftIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.LeftIndex;
		}

		pNode->Head.ParentIndex = pNode->Head.LeftIndex;
		pNode->Head.LeftIndex = pLeftNode->Head.RightIndex;

		TreeNodeType* pLeftRightNode = NULL;
		if(m_NodeBlockTable.GetBlock(pLeftNode->Head.RightIndex, &pLeftRightNode))
			pLeftRightNode->Head.ParentIndex = nodeIndex;

		pLeftNode->Head.RightIndex = nodeIndex;
	}

	void InsertFixup(TreeNodeType* pNode, IndexT* pRootNodeIdx)
	{
		TreeNodeType* pParentNode = NULL;
		while(pNode &&
				IsRed(pNode) &&
				m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode) &&
				IsRed(pParentNode))
		{
			IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

			TreeNodeType* pGrandpaNode = NULL;
			m_NodeBlockTable.GetBlock(pParentNode->Head.ParentIndex, &pGrandpaNode);
			if(pGrandpaNode->Head.LeftIndex == pNode->Head.ParentIndex)
			{
				TreeNodeType* pUncleNode = NULL;
				if(m_NodeBlockTable.GetBlock(pGrandpaNode->Head.RightIndex, &pUncleNode) &&
					IsRed(pUncleNode))
				{
					SetNodeColorBlack(pParentNode);
					SetNodeColorBlack(pUncleNode);
					SetNodeColorRed(pGrandpaNode);
					pNode = pGrandpaNode;
					continue;
				}

				if(nodeIndex == pParentNode->Head.RightIndex)
				{
					SetNodeColorBlack(pNode);
					TreeNodeRotateLeft(pParentNode, pRootNodeIdx);
				}
				else
					SetNodeColorBlack(pParentNode);

				SetNodeColorRed(pGrandpaNode);
				TreeNodeRotateRight(pGrandpaNode, pRootNodeIdx);
			}
			else
			{
				TreeNodeType* pUncleNode = NULL;
				if(m_NodeBlockTable.GetBlock(pGrandpaNode->Head.LeftIndex, &pUncleNode) &&
					IsRed(pUncleNode))
				{
					SetNodeColorBlack(pParentNode);
					SetNodeColorBlack(pUncleNode);
					SetNodeColorRed(pGrandpaNode);
					pNode = pGrandpaNode;
					continue;
				}

				if(nodeIndex == pParentNode->Head.LeftIndex)
				{
					SetNodeColorBlack(pNode);
					TreeNodeRotateRight(pParentNode, pRootNodeIdx);
				}
				else
					SetNodeColorBlack(pParentNode);

				SetNodeColorRed(pGrandpaNode);
				TreeNodeRotateLeft(pGrandpaNode, pRootNodeIdx);
			}
		}

		TreeNodeType* pRootNode = NULL;
		m_NodeBlockTable.GetBlock(*pRootNodeIdx, &pRootNode);
		SetNodeColorBlack(pRootNode);
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
		pNode->Head.Color = NODECOLOR_LEAF;
		pNode->Head.CenterIndex = ValueIdx;

		vNodePtr.push_back(pNode);

		bool error = false;
		for(int i=size-1; i>=0; --i)
		{
			TreeNodeType* pTempNode = NULL;
			IndexT tempNodeIdx = m_NodeBlockTable.AllocateBlock(&pTempNode);
			if(pTempNode == NULL)
			{
				error = true;
				break;
			}

			pTempNode->Head.CenterIndex = nodeIdx;
			pTempNode->Head.Color = NODECOLOR_RED;
			pTempNode->Key = pKeyString[i];

			SetNodeColorBlack(pNode);

			nodeIdx = tempNodeIdx;
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

		pLeafNode->Head.Color = NODECOLOR_LEAF;
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
		if(IsLeaf(pNode))
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
			if(!IsLeaf(pNode))
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

	void PrintNode(TreeNodeType* pNode, std::vector<TreeNodeType*>& vPtr)
	{
		if(!pNode)
			return;

		TreeNodeType* pLeftNode = NULL;
		if(m_NodeBlockTable.GetBlock(pNode->Head.LeftIndex, &pLeftNode))
			PrintNode(pLeftNode, vPtr);

		if(IsLeaf(pNode))
		{
			ValueT* pValue = NULL;
			m_NodeBlockTable.GetBlock(pNode->Head.CenterIndex, &pValue);

			for(typename std::vector<TreeNodeType*>::iterator iter = vPtr.begin();
				iter != vPtr.end();
				++iter)
			{
				printf("%s", KeySerialization<KeyT>::Serialization((*iter)->Key).c_str());
			}
			printf(" Value:%s\n", KeySerialization<ValueT>::Serialization(*pValue).c_str());
		}
		else
		{
			TreeNodeType* pCenterNode = NULL;
			if(m_NodeBlockTable.GetBlock(pNode->Head.CenterIndex, &pCenterNode))
			{
				vPtr.push_back(pNode);
				PrintNode(pCenterNode, vPtr);
			}
			vPtr.pop_back();
		}

		TreeNodeType* pRightNode = NULL;
		if(m_NodeBlockTable.GetBlock(pNode->Head.RightIndex, &pRightNode))
			PrintNode(pRightNode, vPtr);
	}

	MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), TernaryTreeHead<IndexT>, IndexT> m_NodeBlockTable;

};


#endif // define __TERNARYTREE_HPP__
