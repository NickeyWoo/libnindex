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

struct TernaryNodeHead
{
	uint8_t Color;

	uint32_t ParentIndex;
	uint32_t LeftIndex;
	uint32_t RightIndex;

	uint32_t CenterIndex;
} __attribute__((packed));

template<typename KeyT>
struct TernaryNode
{
	TernaryNodeHead Head;
	KeyT Key;
} __attribute__((packed));

struct TernaryTreeHead
{
	uint32_t RootIndex;
} __attribute__((packed));

struct TernaryTreeIteratorImpl
{
	std::vector<uint32_t> LayerIndexVector;
	std::vector<uint32_t> PrefixNodeIndexVector;
};

template<typename ValueT, typename KeyT = char>
class TernaryTree
{
public:
	typedef TernaryTreeIteratorImpl TernaryTreeIterator;

	static TernaryTree<ValueT, KeyT> CreateTernaryTree(uint32_t stringCount, uint32_t avgStringLength)
	{
		std::vector<uint32_t> vec;
		vec.push_back(stringCount * avgStringLength);
		vec.push_back(stringCount);

		TernaryTree<ValueT, KeyT> tt;
		tt.m_NodeBlockTable = MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), TernaryTreeHead>::CreateMultiBlockTable(vec);
		return tt;
	}

	static TernaryTree<ValueT, KeyT> LoadTernaryTree(char* buffer, size_t size, uint32_t stringCount, uint32_t avgStringLength)
	{
		std::vector<uint32_t> vec;
		vec.push_back(stringCount * avgStringLength);
		vec.push_back(stringCount);

		TernaryTree<ValueT, KeyT> tt;
		tt.m_NodeBlockTable = MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), TernaryTreeHead>::LoadMultiBlockTable(buffer, size, vec);
		return tt;
	}

	template<typename StorageT>
	static inline TernaryTree<ValueT, KeyT> LoadTernaryTree(StorageT storage, uint32_t stringCount, uint32_t avgStringLength)
	{
		return LoadTernaryTree(storage.GetStorageBuffer(), storage.GetSize(), stringCount, avgStringLength);
	}

	static inline size_t GetBufferSize(uint32_t stringCount, uint32_t avgStringLength)
	{
		std::vector<uint32_t> vec;
		vec.push_back(stringCount * avgStringLength);
		vec.push_back(stringCount);

		return MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), TernaryTreeHead>::GetBufferSize(vec);
	}

    inline bool Success()
    {
        return m_NodeBlockTable.Success();
    }

    inline float Capacity()
    {
        return m_NodeBlockTable.Capacity();
    }

	inline void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	inline void Clear(const KeyT* pKeyString)
	{
		Length<KeyT> len;
		Clear(pKeyString, len(pKeyString));
	}

	void Clear(const KeyT* pKeyString, size_t size)
	{
		if(!pKeyString)
			return;

		uint32_t* pEmptyIdx;
		uint32_t ParentIdx;

		std::vector<std::pair<TreeNodeType*, uint32_t*> > vNodePtr;

		TernaryTreeHead* pHead = m_NodeBlockTable.GetHead();
		uint32_t* pRootNodeIdx = &pHead->RootIndex;
		for(size_t i=0; i<size; ++i)
		{
			TreeNodeType* pRootNode = FindKey(pKeyString[i], pRootNodeIdx, &pEmptyIdx, &ParentIdx);
			if(pRootNode == NULL)
				return;
			vNodePtr.push_back(std::make_pair(pRootNode, pRootNodeIdx));

			pRootNodeIdx = &pRootNode->Head.CenterIndex;
		}

		TreeNodeType* pLeafNode = FindLeaf(pRootNodeIdx, &pEmptyIdx, &ParentIdx);
		if(pLeafNode == NULL)
			return;
		vNodePtr.push_back(std::make_pair(pLeafNode, pRootNodeIdx));

		ValueT* pValue = NULL;
		if(m_NodeBlockTable.GetBlock(pLeafNode->Head.CenterIndex, &pValue))
			m_NodeBlockTable.ReleaseBlock(pValue);

		for(typename std::vector<std::pair<TreeNodeType*, uint32_t*> >::size_type i = vNodePtr.size() - 1;
			i >= 0; --i)
		{
			TreeNodeType* pNode = vNodePtr[i].first;
			uint32_t* pRootNodeIdx = vNodePtr[i].second;

			bool hasLayerBrother = HasLayerBrother(pNode);
			ClearNode(pNode, pRootNodeIdx);

			if(hasLayerBrother)
				break;
		}
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

		uint32_t* pEmptyIdx;
		uint32_t ParentIdx;

		TernaryTreeHead* pHead = m_NodeBlockTable.GetHead();
		uint32_t* pRootNodeIdx = &pHead->RootIndex;
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

		TernaryTreeHead* pHead = m_NodeBlockTable.GetHead();
		uint32_t* pRootNodeIdx = &pHead->RootIndex;
		for(size_t i=0; i<size; ++i)
		{
			uint32_t* pEmptyIdx;
			uint32_t ParentIdx;

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
				uint32_t idx = pIter->PrefixNodeIndexVector[i];

				TreeNodeType* pNode = NULL;
				m_NodeBlockTable.GetBlock(idx, &pNode);

				pKeyString[i] = pNode->Key;
			}

			for(size_t j=0; i<ret && j<pIter->LayerIndexVector.size(); ++j, ++i)
			{
				uint32_t idx = pIter->LayerIndexVector[j];

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
		TernaryTreeHead* pHead = m_NodeBlockTable.GetHead();

		TreeNodeType* pNode = NULL;
		m_NodeBlockTable.GetBlock(pHead->RootIndex, &pNode);

		std::vector<TreeNodeType*> vPtr;
		PrintNode(pNode, vPtr);
	}

	inline void DumpRBTree(const KeyT* pKeyString)
	{
		Length<KeyT> len;
		DumpRBTree(pKeyString, len(pKeyString));
	}

	void DumpRBTree(const KeyT* pKeyString, size_t size)
	{
		uint32_t* pEmptyIdx;
		uint32_t ParentIdx;

		TernaryTreeHead* pHead = m_NodeBlockTable.GetHead();
		uint32_t* pRootNodeIdx = &pHead->RootIndex;
		for(size_t i=0; i<size; ++i)
		{
			TreeNodeType* pRootNode = FindKey(pKeyString[i], pRootNodeIdx, &pEmptyIdx, &ParentIdx);
			if(pRootNode == NULL)
				return;

			pRootNodeIdx = &pRootNode->Head.CenterIndex;
		}

		std::vector<bool> flags;
		flags.push_back(false);

		TreeNodeType* pNode = NULL;
		m_NodeBlockTable.GetBlock(*pRootNodeIdx, &pNode);
		PrintRBNode(pNode, 0, false, flags);
	}

	inline void Dump()
	{
		m_NodeBlockTable.Dump();
	}

protected:
	typedef TernaryNode<KeyT> TreeNodeType;

	inline bool IsLeaf(TreeNodeType* pNode)
	{
		return (pNode->Head.Color & NODECOLOR_LEAF) == NODECOLOR_LEAF;
	}

	inline bool IsBlack(TreeNodeType* pNode)
	{
		return (pNode->Head.Color & NODECOLOR_BLACK) == NODECOLOR_BLACK;
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

	void ClearFixup(TreeNodeType* pNode, TreeNodeType* pParentNode, uint32_t* pRootNodeIdx)
	{
		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);
		while((!pNode || IsBlack(pNode)) && nodeIndex != *pRootNodeIdx)
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
			{
				TreeNodeType* pBrotherNode = NULL;
				m_NodeBlockTable.GetBlock(pParentNode->Head.RightIndex, &pBrotherNode);
				if(IsRed(pBrotherNode))
				{
					SetNodeColorBlack(pBrotherNode);
					SetNodeColorRed(pParentNode);
					TreeNodeRotateLeft(pParentNode, pRootNodeIdx);

					m_NodeBlockTable.GetBlock(pParentNode->Head.RightIndex, &pBrotherNode);
				}

				TreeNodeType* pBrotherLeftNode = NULL;
				m_NodeBlockTable.GetBlock(pBrotherNode->Head.LeftIndex, &pBrotherLeftNode);

				TreeNodeType* pBrotherRightNode = NULL;
				m_NodeBlockTable.GetBlock(pBrotherNode->Head.RightIndex, &pBrotherRightNode);
				if((!pBrotherLeftNode || IsBlack(pBrotherLeftNode)) &&
					(!pBrotherRightNode || IsBlack(pBrotherRightNode)))
				{
					SetNodeColorRed(pBrotherNode);

					nodeIndex = m_NodeBlockTable.GetBlockID(pParentNode);
					pNode = pParentNode;
					m_NodeBlockTable.GetBlock(pParentNode->Head.ParentIndex, &pParentNode);
				}
				else
				{
					if(!pBrotherRightNode || IsBlack(pBrotherRightNode))
					{
						SetNodeColorBlack(pBrotherLeftNode);
						SetNodeColorRed(pBrotherNode);
						TreeNodeRotateRight(pBrotherNode, pRootNodeIdx);

						pBrotherRightNode = pBrotherNode;
						pBrotherNode = pBrotherLeftNode;
					}

					if(IsBlack(pParentNode))
						SetNodeColorBlack(pBrotherNode);
					else
						SetNodeColorRed(pBrotherNode);
					SetNodeColorBlack(pParentNode);
					SetNodeColorBlack(pBrotherRightNode);
					TreeNodeRotateLeft(pParentNode, pRootNodeIdx);

					m_NodeBlockTable.GetBlock(*pRootNodeIdx, &pNode);
					break;
				}
			}
			else
			{
				TreeNodeType* pBrotherNode = NULL;
				m_NodeBlockTable.GetBlock(pParentNode->Head.LeftIndex, &pBrotherNode);
				if(IsRed(pBrotherNode))
				{
					SetNodeColorBlack(pBrotherNode);
					SetNodeColorRed(pParentNode);
					TreeNodeRotateRight(pParentNode, pRootNodeIdx);

					m_NodeBlockTable.GetBlock(pParentNode->Head.LeftIndex, &pBrotherNode);
				}

				TreeNodeType* pBrotherLeftNode = NULL;
				m_NodeBlockTable.GetBlock(pBrotherNode->Head.LeftIndex, &pBrotherLeftNode);

				TreeNodeType* pBrotherRightNode = NULL;
				m_NodeBlockTable.GetBlock(pBrotherNode->Head.RightIndex, &pBrotherRightNode);
				if((!pBrotherLeftNode || IsBlack(pBrotherLeftNode)) &&
					(!pBrotherRightNode || IsBlack(pBrotherRightNode)))
				{
					SetNodeColorRed(pBrotherNode);

					nodeIndex = m_NodeBlockTable.GetBlockID(pParentNode);
					pNode = pParentNode;
					m_NodeBlockTable.GetBlock(pParentNode->Head.ParentIndex, &pParentNode);
				}
				else
				{
					if(!pBrotherLeftNode || IsBlack(pBrotherLeftNode))
					{
						SetNodeColorBlack(pBrotherRightNode);
						SetNodeColorRed(pBrotherNode);
						TreeNodeRotateLeft(pBrotherNode, pRootNodeIdx);

						pBrotherLeftNode = pBrotherNode;
						pBrotherNode = pBrotherRightNode;
					}

					if(IsBlack(pParentNode))
						SetNodeColorBlack(pBrotherNode);
					else
						SetNodeColorRed(pBrotherNode);
					SetNodeColorBlack(pParentNode);
					SetNodeColorBlack(pBrotherLeftNode);
					TreeNodeRotateRight(pParentNode, pRootNodeIdx);

					m_NodeBlockTable.GetBlock(*pRootNodeIdx, &pNode);
					break;
				}
			}
		}

		if(pNode)
			SetNodeColorBlack(pNode);
	}

	void TreeNodeTransplantLeft(TreeNodeType* pNode, uint32_t nodeIndex, uint32_t* pRootNodeIdx)
	{
		if(!pNode || !pRootNodeIdx)
			return;

		TreeNodeType* pLeftNode = NULL;
		if(m_NodeBlockTable.GetBlock(pNode->Head.LeftIndex, &pLeftNode))
			pLeftNode->Head.ParentIndex = pNode->Head.ParentIndex;

		TreeNodeType* pParentNode = NULL;
		if(m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode))
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.LeftIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.LeftIndex;
		}
		else
			*pRootNodeIdx = pNode->Head.LeftIndex;
	}

	void TreeNodeTransplantRight(TreeNodeType* pNode, uint32_t nodeIndex, uint32_t* pRootNodeIdx)
	{
		if(!pNode || !pRootNodeIdx)
			return;

		TreeNodeType* pRightNode = NULL;
		if(m_NodeBlockTable.GetBlock(pNode->Head.RightIndex, &pRightNode))
			pRightNode->Head.ParentIndex = pNode->Head.ParentIndex;

		TreeNodeType* pParentNode = NULL;
		if(m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode))
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.RightIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.RightIndex;
		}
		else
			*pRootNodeIdx = pNode->Head.RightIndex;
	}

	void ClearOneChildNode(TreeNodeType* pNode, uint32_t* pRootNodeIdx)
	{
		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

		bool needFixup = IsBlack(pNode);

		TreeNodeType* pFixParentNode = NULL;
		m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pFixParentNode);

		TreeNodeType* pFixNode = NULL;
		TreeNodeType* pLeftNode = NULL;
		TreeNodeType* pRightNode = NULL;
		m_NodeBlockTable.GetBlock(pNode->Head.LeftIndex, &pLeftNode);
		m_NodeBlockTable.GetBlock(pNode->Head.RightIndex, &pRightNode);

		if(pLeftNode == NULL)
		{
			TreeNodeTransplantRight(pNode, nodeIndex, pRootNodeIdx);
			pFixNode = pRightNode;
		}
		else
		{
			TreeNodeTransplantLeft(pNode, nodeIndex, pRootNodeIdx);
			pFixNode = pLeftNode;
		}

		m_NodeBlockTable.ReleaseBlock(pNode);

		if(needFixup)
			ClearFixup(pFixNode, pFixParentNode, pRootNodeIdx);
	}

	void ClearNode(TreeNodeType* pNode, uint32_t* pRootNodeIdx)
	{
		if(!pNode)
			return;

		TreeNodeType* pLeftNode = NULL;
		TreeNodeType* pRightNode = NULL;
		if(!m_NodeBlockTable.GetBlock(pNode->Head.LeftIndex, &pLeftNode) ||
			!m_NodeBlockTable.GetBlock(pNode->Head.RightIndex, &pRightNode))
		{
			ClearOneChildNode(pNode, pRootNodeIdx);
		}
		else
		{
			TreeNodeType* pMiniNode = TreeNodeMinimum(pRightNode);

			// clone key and clear mini node
			pNode->Key = pMiniNode->Key;

			ClearOneChildNode(pMiniNode, pRootNodeIdx);
		}
	}

	bool HasLayerBrother(TreeNodeType* pNode)
	{
		TreeNodeType* pParentNode = NULL;
		TreeNodeType* pLeftNode = NULL;
		TreeNodeType* pRightNode = NULL;

		if(m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode) ||
			m_NodeBlockTable.GetBlock(pNode->Head.LeftIndex, &pLeftNode) ||
			m_NodeBlockTable.GetBlock(pNode->Head.RightIndex, &pRightNode))
			return true;
		else
			return false;
	}

	void MinimumString(TernaryTreeIterator* pIter, uint32_t RootIndex)
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
			uint32_t cur = pIter->LayerIndexVector.back();
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

	void TreeNodeRotateLeft(TreeNodeType* pNode, uint32_t* pRootNodeIdx)
	{
		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

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

	void TreeNodeRotateRight(TreeNodeType* pNode, uint32_t* pRootNodeIdx)
	{
		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

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

	void InsertFixup(TreeNodeType* pNode, uint32_t* pRootNodeIdx)
	{
		TreeNodeType* pParentNode = NULL;
		while(pNode &&
				IsRed(pNode) &&
				m_NodeBlockTable.GetBlock(pNode->Head.ParentIndex, &pParentNode) &&
				IsRed(pParentNode))
		{
			uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

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

	ValueT* InsertString(const KeyT* pKeyString, size_t size, uint32_t* pEmptyIdx, uint32_t ParentIdx, uint32_t* pRootNodeIdx)
	{
		ValueT* pValue = NULL;
		uint32_t ValueIdx = m_NodeBlockTable.AllocateBlock(&pValue);
		if(!pValue)
			return NULL;

		std::vector<TreeNodeType*> vNodePtr;

		TreeNodeType* pNode = NULL;
		uint32_t nodeIdx = m_NodeBlockTable.AllocateBlock(&pNode);
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
			uint32_t tempNodeIdx = m_NodeBlockTable.AllocateBlock(&pTempNode);
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

	ValueT* InsertLeaf(uint32_t* pEmptyIdx, uint32_t ParentIdx, uint32_t* pRootNodeIdx)
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

	TreeNodeType* FindLeaf(uint32_t* pRootNodeIdx, uint32_t** ppEmptyIdx, uint32_t* pParentIdx)
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

	TreeNodeType* FindKey(const KeyT key, uint32_t* pRootNodeIdx, uint32_t** ppEmptyIdx, uint32_t* pParentIdx)
	{
		if(!pRootNodeIdx || !ppEmptyIdx || !pParentIdx)
			return NULL;

		*ppEmptyIdx = pRootNodeIdx;
		*pParentIdx = 0;

		TreeNodeType* pNode = NULL;
		uint32_t nodeIdx = *pRootNodeIdx;
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

	void PrintRBNode(TreeNodeType* node, std::vector<bool>::size_type layer, bool isRight, std::vector<bool>& flags)
	{
		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(node);
		if(layer > 0)
		{
			for(uint32_t c=0; c<layer; ++c)
			{
				if(flags.at(c))
					printf("\033[37m|    \033[0m");
				else
					printf("     ");
			}
			if(isRight)
				printf("\033[37m\\-\033[0m");
			else
				printf("\033[37m|-\033[0m");
			if(node)
				printf("\033[%sm%snode: (%s)[%s][p%u:c%u:l%u:r%u]\033[0m\n",
							IsRed(node)?"31":"34",
							isRight?"r":"l",
							IsLeaf(node)?"leaf":KeySerialization<KeyT>::Serialization(node->Key).c_str(),
							IsBlack(node)?"black":"red",
							node->Head.ParentIndex,
							nodeIndex,
							node->Head.LeftIndex,
							node->Head.RightIndex);
			else
			{
				printf("\033[37m%snode: (null)\033[0m\n", isRight?"r":"l");
				return;
			}
		}
		else
		{
			printf("\033[37m\\-\033[0m");
			if(node)
				printf("\033[%smroot: (%s)[%s][p%u:c%u:l%u:r%u]\033[0m\n",
							IsRed(node)?"31":"34",
							IsLeaf(node)?"leaf":KeySerialization<KeyT>::Serialization(node->Key).c_str(),
							IsBlack(node)?"black":"red",
							node->Head.ParentIndex,
							nodeIndex,
							node->Head.LeftIndex,
							node->Head.RightIndex);
			else
			{
				printf("\033[37mroot: (null)\033[0m\n");
				return;
			}
		}
	
		if(isRight)
			flags.at(layer) = false;
		else if(layer > 0)
			flags.at(layer) = true;

		flags.push_back(true);
		TreeNodeType* leftNode = NULL;
		m_NodeBlockTable.GetBlock(node->Head.LeftIndex, &leftNode);
		PrintRBNode(leftNode, layer+1, false, flags);

		TreeNodeType* rightNode = NULL;
		m_NodeBlockTable.GetBlock(node->Head.RightIndex, &rightNode);
		PrintRBNode(rightNode, layer+1, true, flags);
	}

	MultiBlockTable<TYPELIST_2(TreeNodeType, ValueT), TernaryTreeHead> m_NodeBlockTable;

};


#endif // define __TERNARYTREE_HPP__
