/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.24
 *
*--*/
#ifndef __RBTREE_HPP__
#define __RBTREE_HPP__

#include <utility>
#include <vector>
#include "keyutility.hpp"
#include "blocktable.hpp"

#define RBTREE_NODECOLOR_RED		0
#define RBTREE_NODECOLOR_BLACK		1

template<typename IndexT>
struct RBTreeNodeHead
{
	uint8_t Color;
	uint32_t LeftCount;
	IndexT ParentIndex;
	IndexT LeftIndex;
	IndexT RightIndex;
} __attribute__((packed));

template<typename KeyT, typename ValueT, typename IndexT>
struct RBTreeNode
{
	RBTreeNodeHead<IndexT> Head;
	KeyT Key;
	ValueT Value;
} __attribute__((packed));

template<typename IndexT>
struct RBTreeHead
{
	IndexT RootIndex;
	uint64_t Flags;
} __attribute__((packed));

template<typename IndexT>
struct RBTreeIteratorImpl
{
	IndexT Index;

	bool operator == (const RBTreeIteratorImpl<IndexT>& iter)
	{
		return (Index == iter.Index);
	}

	bool operator != (const RBTreeIteratorImpl<IndexT>& iter)
	{
		return (Index != iter.Index);
	}
};

template<typename KeyT, typename ValueT, typename IndexT = uint32_t>
class RBTree
{
public:
	typedef RBTreeIteratorImpl<IndexT> RBTreeIterator;

	static RBTree<KeyT, ValueT, IndexT> CreateRBTree(IndexT size)
	{
		RBTree<KeyT, ValueT, IndexT> rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNode<KeyT, ValueT, IndexT>, RBTreeHead<IndexT>, IndexT>::CreateBlockTable(size);
		return rbt;
	}

	static RBTree<KeyT, ValueT, IndexT> LoadRBTree(char* buffer, size_t size)
	{
		RBTree<KeyT, ValueT, IndexT> rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNode<KeyT, ValueT, IndexT>, RBTreeHead<IndexT>, IndexT>::LoadBlockTable(buffer, size);
		return rbt;
	}

	template<typename StorageT>
	static RBTree<KeyT, ValueT, IndexT> LoadRBTree(StorageT storage)
	{
		RBTree<KeyT, ValueT, IndexT> rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNode<KeyT, ValueT, IndexT>, RBTreeHead<IndexT>, IndexT>::LoadBlockTable(storage);
		return rbt;
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return BlockTable<RBTreeNode<KeyT, ValueT, IndexT>, RBTreeHead<IndexT>, IndexT>::GetBufferSize(size);
	}

	void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	void Clear(KeyT key)
	{
		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNode<KeyT, ValueT, IndexT>* node = m_NodeBlockTable[pHead->RootIndex];
		while(node != NULL)
		{
			int result = KeyCompare<KeyT>::Compare(node->Key, key);
			if(result == 0)
			{
				ClearNode(node, pHead);
				return;
			}
			else if(result > 0)
				node = m_NodeBlockTable[node->Head.LeftIndex];
			else
				node = m_NodeBlockTable[node->Head.RightIndex];
		}
	}

	ValueT* Minimum(KeyT* pKey = NULL)
	{
		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNode<KeyT, ValueT, IndexT>* node = TreeNodeMinimum(m_NodeBlockTable[pHead->RootIndex]);
		if(node)
		{
			if(pKey)
				memcpy(pKey, &node->Key, sizeof(KeyT));
			return &node->Value;
		}
		return NULL;
	}

	ValueT* Maximum(KeyT* pKey = NULL)
	{
		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNode<KeyT, ValueT, IndexT>* node = TreeNodeMaximum(m_NodeBlockTable[pHead->RootIndex]);
		if(node)
		{
			if(pKey)
				memcpy(pKey, &node->Key, sizeof(KeyT));
			return &node->Value;
		}
		return NULL;
	}

	uint32_t Count(RBTreeIterator iter)
	{
		uint32_t count = 0;

		IndexT nodeIndex = iter->Index;
		RBTreeNode<KeyT, ValueT, IndexT>* node = m_NodeBlockTable[nodeIndex];
		while(node != NULL)
		{
			count += node->Head.LeftCount + 1;

			RBTreeNode<KeyT, ValueT, IndexT>* parentNode = NULL;
			while((parentNode = m_NodeBlockTable[node->Head.ParentIndex]) && parentNode->Head.LeftIndex == nodeIndex)
			{
				nodeIndex = node->Head.ParentIndex;
				node = parentNode;
			}

			if(parentNode == NULL)
				break;

			nodeIndex = node->Head.ParentIndex;
			node = parentNode;
		}
		return count;
	}

	uint32_t Count(RBTreeIterator iterBegin, RBTreeIterator iterEnd)
	{
		uint32_t c1 = Count(iterBegin);
		uint32_t c2 = Count(iterEnd);
		if(c1 > c2)
			return 0;
		else
			return c2 - c1;
	}

	RBTreeIterator Iterator()
	{
		RBTreeIterator iter;

		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNode<KeyT, ValueT, IndexT>* node = TreeNodeMinimum(m_NodeBlockTable[pHead->RootIndex]);
		if(node)
			iter.Index = m_NodeBlockTable.GetBlockID(node);
		else
			iter.Index = pHead->RootIndex;
		return iter;
	}

	RBTreeIterator Iterator(KeyT key)
	{
		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();

		RBTreeIterator iter;
		iter.Index = pHead->RootIndex;

		RBTreeNode<KeyT, ValueT, IndexT>* node = m_NodeBlockTable[pHead->RootIndex];
		while(node != NULL)
		{
			iter.Index = m_NodeBlockTable.GetBlockID(node);
			int result = KeyCompare<KeyT>::Compare(node->Key, key);
			if(result == 0)
			{
				return iter;
			}
			else if(result > 0)
			{
				node = m_NodeBlockTable[node->Head.LeftIndex];
			}
			else
			{
				if(m_NodeBlockTable[node->Head.RightIndex])
					node = m_NodeBlockTable[node->Head.RightIndex];
				else
				{
					RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = NULL;
					while((pParentNode = m_NodeBlockTable[node->Head.ParentIndex]) && pParentNode->Head.RightIndex == iter.Index)
					{
						iter.Index = node->Head.ParentIndex;
						node = pParentNode;
					}
					iter.Index = node->Head.ParentIndex;
					return iter;
				}
			}
		}
		return iter;
	}

	ValueT* Next(RBTreeIterator* pIter, KeyT* pKey = NULL)
	{
		RBTreeNode<KeyT, ValueT, IndexT>* pNode = m_NodeBlockTable[pIter->Index];
		if(pNode)
		{
			RBTreeNode<KeyT, ValueT, IndexT>* pRightNode = m_NodeBlockTable[pNode->Head.RightIndex];
			if(pRightNode == NULL)
			{
				IndexT currentNodeIndex = pIter->Index;
				RBTreeNode<KeyT, ValueT, IndexT>* pCurrentNode = pNode;
				RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = NULL;
				while((pParentNode = m_NodeBlockTable[pCurrentNode->Head.ParentIndex]) && pParentNode->Head.RightIndex == currentNodeIndex)
				{
					currentNodeIndex = pCurrentNode->Head.ParentIndex;
					pCurrentNode = pParentNode;
				}
				pIter->Index = pCurrentNode->Head.ParentIndex;
			}
			else
			{
				RBTreeNode<KeyT, ValueT, IndexT>* pMiniNode = TreeNodeMinimum(pRightNode);
				pIter->Index = m_NodeBlockTable.GetBlockID(pMiniNode);
			}
	
			if(pKey)
				memcpy(pKey, &pNode->Key, sizeof(KeyT));
			return &pNode->Value;
		}
		else
			return NULL;
	}

	ValueT* Hash(KeyT key, bool isNew = false)
	{
		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		IndexT CurIdx = pHead->RootIndex;
		IndexT ParentIdx = pHead->RootIndex;

		IndexT* pEmptyIdx = &pHead->RootIndex;
		RBTreeNode<KeyT, ValueT, IndexT>* node = m_NodeBlockTable[pHead->RootIndex];

		while(node != NULL)
		{
			int result = KeyCompare<KeyT>::Compare(node->Key, key);
			if(result == 0)
				return &node->Value;
			else if(result > 0)
			{
				ParentIdx = CurIdx;
				CurIdx = node->Head.LeftIndex;

				pEmptyIdx = &node->Head.LeftIndex;
				node = m_NodeBlockTable[node->Head.LeftIndex];
			}
			else
			{
				ParentIdx = CurIdx;
				CurIdx = node->Head.RightIndex;

				pEmptyIdx = &node->Head.RightIndex;
				node = m_NodeBlockTable[node->Head.RightIndex];
			}
		}
		if(node == NULL && isNew)
		{
			*pEmptyIdx = m_NodeBlockTable.AllocateBlock();
			node = m_NodeBlockTable[*pEmptyIdx];
			if(node == NULL)
				return NULL;

			memset(node, 0, sizeof(RBTreeNode<KeyT, ValueT, IndexT>));
			node->Head.ParentIndex = ParentIdx;
			memcpy(&node->Key, &key, sizeof(KeyT));

			InsertLeftCountFixup(*pEmptyIdx, node);
			InsertFixup(node, pHead);
			return &node->Value;
		}
		return NULL;
	}

	inline void Dump()
	{
		m_NodeBlockTable.Dump();
	}

	void DumpTree()
	{
		std::vector<bool> flags;
		flags.push_back(false);

		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNode<KeyT, ValueT, IndexT>* node = m_NodeBlockTable[pHead->RootIndex];
		PrintNode(node, 0, false, flags);
	}

	RBTree()
	{
	}
	
protected:
	inline RBTreeNode<KeyT, ValueT, IndexT>* TreeNodeMaximum(RBTreeNode<KeyT, ValueT, IndexT>* pNode)
	{
		while(pNode && m_NodeBlockTable[pNode->Head.RightIndex])
			pNode = m_NodeBlockTable[pNode->Head.RightIndex];
		return pNode;
	}

	inline RBTreeNode<KeyT, ValueT, IndexT>* TreeNodeMinimum(RBTreeNode<KeyT, ValueT, IndexT>* pNode)
	{
		while(pNode && m_NodeBlockTable[pNode->Head.LeftIndex])
			pNode = m_NodeBlockTable[pNode->Head.LeftIndex];
		return pNode;
	}

	void TreeNodeTransplantRight(RBTreeNode<KeyT, ValueT, IndexT>* pNode, IndexT nodeIndex, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || m_NodeBlockTable[pNode->Head.LeftIndex] || !pHead)
			return;

		RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		RBTreeNode<KeyT, ValueT, IndexT>* pRightNode = m_NodeBlockTable[pNode->Head.RightIndex];

		if(pRightNode)
			pRightNode->Head.ParentIndex = pNode->Head.ParentIndex;

		if(pParentNode)
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.RightIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.RightIndex;
		}
		else
			pHead->RootIndex = pNode->Head.RightIndex;
	}

	void TreeNodeTransplantLeft(RBTreeNode<KeyT, ValueT, IndexT>* pNode, IndexT nodeIndex, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || m_NodeBlockTable[pNode->Head.RightIndex] || !pHead)
			return;

		RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		RBTreeNode<KeyT, ValueT, IndexT>* pLeftNode = m_NodeBlockTable[pNode->Head.LeftIndex];

		if(pLeftNode)
			pLeftNode->Head.ParentIndex = pNode->Head.ParentIndex;

		if(pParentNode)
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.LeftIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.LeftIndex;
		}
		else
			pHead->RootIndex = pNode->Head.LeftIndex;
	}

	void ClearLeftCountFixup(IndexT nodeIndex, RBTreeNode<KeyT, ValueT, IndexT>* pNode)
	{
		RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = NULL;
		while(pNode && (pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex]))
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				--pParentNode->Head.LeftCount;

			nodeIndex = pNode->Head.ParentIndex;
			pNode = pParentNode;
		}
	}

	void ClearOneChildNode(RBTreeNode<KeyT, ValueT, IndexT>* pNode, RBTreeHead<IndexT>* pHead)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);
		ClearLeftCountFixup(nodeIndex, pNode);

		bool needFixup = (pNode->Head.Color == RBTREE_NODECOLOR_BLACK);

		RBTreeNode<KeyT, ValueT, IndexT>* pFixNode = NULL;
		RBTreeNode<KeyT, ValueT, IndexT>* pFixParentNode = NULL;

		if(m_NodeBlockTable[pNode->Head.LeftIndex] == NULL)
		{
			TreeNodeTransplantRight(pNode, nodeIndex, pHead);
			pFixNode = m_NodeBlockTable[pNode->Head.RightIndex];
			pFixParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		}
		else
		{
			TreeNodeTransplantLeft(pNode, nodeIndex, pHead);
			pFixNode = m_NodeBlockTable[pNode->Head.LeftIndex];
			pFixParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		}

		m_NodeBlockTable.ReleaseBlock(nodeIndex);

		if(needFixup)
			ClearFixup(pFixNode, pFixParentNode, pHead);
	}

	void ClearNode(RBTreeNode<KeyT, ValueT, IndexT>* pNode, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode)
			return;

		if(m_NodeBlockTable[pNode->Head.LeftIndex] == NULL ||
			m_NodeBlockTable[pNode->Head.RightIndex] == NULL)
		{
			ClearOneChildNode(pNode, pHead);
		}
		else
		{
			RBTreeNode<KeyT, ValueT, IndexT>* pMiniNode = TreeNodeMinimum(m_NodeBlockTable[pNode->Head.RightIndex]);

			// clone Key and Data, then Clear MiniNode.
			memcpy(&pNode->Key, &pMiniNode->Key, sizeof(KeyT));
			memcpy(&pNode->Value, &pMiniNode->Value, sizeof(ValueT));

			ClearOneChildNode(pMiniNode, pHead);
		}
	}

	void ClearFixup(RBTreeNode<KeyT, ValueT, IndexT>* pNode, RBTreeNode<KeyT, ValueT, IndexT>* pParentNode, RBTreeHead<IndexT>* pHead)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);
		while((!pNode || pNode->Head.Color == RBTREE_NODECOLOR_BLACK) && nodeIndex != pHead->RootIndex)
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
			{
				RBTreeNode<KeyT, ValueT, IndexT>* pBrotherNode = m_NodeBlockTable[pParentNode->Head.RightIndex];
				if(pBrotherNode->Head.Color == RBTREE_NODECOLOR_RED)
				{
					pBrotherNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pParentNode->Head.Color = RBTREE_NODECOLOR_RED;
					TreeNodeRotateLeft(pParentNode, pHead);

					pBrotherNode = m_NodeBlockTable[pParentNode->Head.RightIndex];
				}

				RBTreeNode<KeyT, ValueT, IndexT>* pBrotherLeftNode = m_NodeBlockTable[pBrotherNode->Head.LeftIndex];
				RBTreeNode<KeyT, ValueT, IndexT>* pBrotherRightNode = m_NodeBlockTable[pBrotherNode->Head.RightIndex];
				if((!pBrotherLeftNode || pBrotherLeftNode->Head.Color == RBTREE_NODECOLOR_BLACK) &&
					(!pBrotherRightNode || pBrotherRightNode->Head.Color == RBTREE_NODECOLOR_BLACK))
				{
					pBrotherNode->Head.Color = RBTREE_NODECOLOR_RED;

					nodeIndex = m_NodeBlockTable.GetBlockID(pParentNode);
					pNode = pParentNode;
					pParentNode = m_NodeBlockTable[pParentNode->Head.ParentIndex];
				}
				else
				{
					if(!pBrotherRightNode || pBrotherRightNode->Head.Color == RBTREE_NODECOLOR_BLACK)
					{
						pBrotherLeftNode->Head.Color = RBTREE_NODECOLOR_BLACK;
						pBrotherNode->Head.Color = RBTREE_NODECOLOR_RED;
						TreeNodeRotateRight(pBrotherNode, pHead);

						pBrotherRightNode = pBrotherNode;
						pBrotherNode = pBrotherLeftNode;
					}
					pBrotherNode->Head.Color = pParentNode->Head.Color;
					pParentNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pBrotherRightNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					TreeNodeRotateLeft(pParentNode, pHead);

					pNode = m_NodeBlockTable[pHead->RootIndex];
					break;
				}
			}
			else
			{
				RBTreeNode<KeyT, ValueT, IndexT>* pBrotherNode = m_NodeBlockTable[pParentNode->Head.LeftIndex];
				if(pBrotherNode->Head.Color == RBTREE_NODECOLOR_RED)
				{
					pBrotherNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pParentNode->Head.Color = RBTREE_NODECOLOR_RED;
					TreeNodeRotateRight(pParentNode, pHead);

					pBrotherNode = m_NodeBlockTable[pParentNode->Head.LeftIndex];
				}

				RBTreeNode<KeyT, ValueT, IndexT>* pBrotherLeftNode = m_NodeBlockTable[pBrotherNode->Head.LeftIndex];
				RBTreeNode<KeyT, ValueT, IndexT>* pBrotherRightNode = m_NodeBlockTable[pBrotherNode->Head.RightIndex];
				if((!pBrotherLeftNode || pBrotherLeftNode->Head.Color == RBTREE_NODECOLOR_BLACK) &&
					(!pBrotherRightNode || pBrotherRightNode->Head.Color == RBTREE_NODECOLOR_BLACK))
				{
					pBrotherNode->Head.Color = RBTREE_NODECOLOR_RED;

					nodeIndex = m_NodeBlockTable.GetBlockID(pParentNode);
					pNode = pParentNode;
					pParentNode = m_NodeBlockTable[pParentNode->Head.ParentIndex];
				}
				else
				{
					if(!pBrotherLeftNode || pBrotherLeftNode->Head.Color == RBTREE_NODECOLOR_BLACK)
					{
						pBrotherRightNode->Head.Color = RBTREE_NODECOLOR_BLACK;
						pBrotherNode->Head.Color = RBTREE_NODECOLOR_RED;
						TreeNodeRotateLeft(pBrotherNode, pHead);

						pBrotherLeftNode = pBrotherNode;
						pBrotherNode = pBrotherRightNode;
					}
					pBrotherNode->Head.Color = pParentNode->Head.Color;
					pParentNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pBrotherLeftNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					TreeNodeRotateRight(pParentNode, pHead);

					pNode = m_NodeBlockTable[pHead->RootIndex];
					break;
				}
			}
		}

		if(pNode)
			pNode->Head.Color = RBTREE_NODECOLOR_BLACK;
	}

	void InsertLeftCountFixup(IndexT nodeIndex, RBTreeNode<KeyT, ValueT, IndexT>* pNode)
	{
		RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = NULL;
		while(pNode && (pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex]))
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				++pParentNode->Head.LeftCount;

			nodeIndex = pNode->Head.ParentIndex;
			pNode = pParentNode;
		}
	}

	void InsertFixup(RBTreeNode<KeyT, ValueT, IndexT>* pNode, RBTreeHead<IndexT>* pHead)
	{
		RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = NULL;
		while(pNode && 
			  pNode->Head.Color == RBTREE_NODECOLOR_RED &&
			  (pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex]) &&
			  pParentNode->Head.Color == RBTREE_NODECOLOR_RED)
		{
			IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

			RBTreeNode<KeyT, ValueT, IndexT>* pGrandpaNode = m_NodeBlockTable[pParentNode->Head.ParentIndex];
			if(pGrandpaNode->Head.LeftIndex == pNode->Head.ParentIndex)
			{
				RBTreeNode<KeyT, ValueT, IndexT>* pUncleNode = m_NodeBlockTable[pGrandpaNode->Head.RightIndex];
				if(pUncleNode && pUncleNode->Head.Color == RBTREE_NODECOLOR_RED)
				{
					pParentNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pUncleNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pGrandpaNode->Head.Color = RBTREE_NODECOLOR_RED;
					pNode = pGrandpaNode;
					continue;
				}

				if(nodeIndex == pParentNode->Head.RightIndex)
				{
					pNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					TreeNodeRotateLeft(pParentNode, pHead);
				}
				else
					pParentNode->Head.Color = RBTREE_NODECOLOR_BLACK;

				pGrandpaNode->Head.Color = RBTREE_NODECOLOR_RED;
				TreeNodeRotateRight(pGrandpaNode, pHead);
			}
			else
			{
				RBTreeNode<KeyT, ValueT, IndexT>* pUncleNode = m_NodeBlockTable[pGrandpaNode->Head.LeftIndex];
				if(pUncleNode && pUncleNode->Head.Color == RBTREE_NODECOLOR_RED)
				{
					pParentNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pUncleNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pGrandpaNode->Head.Color = RBTREE_NODECOLOR_RED;
					pNode = pGrandpaNode;
					continue;
				}

				if(nodeIndex == pParentNode->Head.LeftIndex)
				{
					pNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					TreeNodeRotateRight(pParentNode, pHead);
				}
				else
					pParentNode->Head.Color = RBTREE_NODECOLOR_BLACK;

				pGrandpaNode->Head.Color = RBTREE_NODECOLOR_RED;
				TreeNodeRotateLeft(pGrandpaNode, pHead);
			}
		}
		m_NodeBlockTable[pHead->RootIndex]->Head.Color = RBTREE_NODECOLOR_BLACK;
	}

	void RotateLeftCountFixup(RBTreeNode<KeyT, ValueT, IndexT>* pNode)
	{
		if(!pNode)
			return;

		uint32_t count = 0;
		RBTreeNode<KeyT, ValueT, IndexT>* pTempNode = m_NodeBlockTable[pNode->Head.LeftIndex];
		while(pTempNode)
		{
			count += pTempNode->Head.LeftCount + 1;
			pTempNode = m_NodeBlockTable[pTempNode->Head.RightIndex];
		}

		pNode->Head.LeftCount = count;
	}

	void TreeNodeRotateLeft(RBTreeNode<KeyT, ValueT, IndexT>* pNode, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || !pHead)
			return;

		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

		RBTreeNode<KeyT, ValueT, IndexT>* pRightNode = m_NodeBlockTable[pNode->Head.RightIndex];
		if(!pRightNode)
			return;
		pRightNode->Head.ParentIndex = pNode->Head.ParentIndex;

		RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		if(pParentNode == NULL)
			pHead->RootIndex = pNode->Head.RightIndex;
		else
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.RightIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.RightIndex;
		}

		pNode->Head.ParentIndex = pNode->Head.RightIndex;
		pNode->Head.RightIndex = pRightNode->Head.LeftIndex;

		RBTreeNode<KeyT, ValueT, IndexT>* pRightLeftNode = m_NodeBlockTable[pRightNode->Head.LeftIndex];
		if(pRightLeftNode)
			pRightLeftNode->Head.ParentIndex = nodeIndex;

		pRightNode->Head.LeftIndex = nodeIndex;

		RotateLeftCountFixup(pRightNode);
	}

	void TreeNodeRotateRight(RBTreeNode<KeyT, ValueT, IndexT>* pNode, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || !pHead)
			return;

		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

		RBTreeNode<KeyT, ValueT, IndexT>* pLeftNode = m_NodeBlockTable[pNode->Head.LeftIndex];
		if(!pLeftNode)
			return;
		pLeftNode->Head.ParentIndex = pNode->Head.ParentIndex;

		RBTreeNode<KeyT, ValueT, IndexT>* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		if(pParentNode == NULL)
			pHead->RootIndex = pNode->Head.LeftIndex;
		else
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				pParentNode->Head.LeftIndex = pNode->Head.LeftIndex;
			else
				pParentNode->Head.RightIndex = pNode->Head.LeftIndex;
		}

		pNode->Head.ParentIndex = pNode->Head.LeftIndex;
		pNode->Head.LeftIndex = pLeftNode->Head.RightIndex;

		RBTreeNode<KeyT, ValueT, IndexT>* pLeftRightNode = m_NodeBlockTable[pLeftNode->Head.RightIndex];
		if(pLeftRightNode)
			pLeftRightNode->Head.ParentIndex = nodeIndex;

		pLeftNode->Head.RightIndex = nodeIndex;

		RotateLeftCountFixup(pNode);
	}

	void PrintNode(RBTreeNode<KeyT, ValueT, IndexT>* node, std::vector<bool>::size_type layer, bool isRight, std::vector<bool>& flags)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(node);
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
				printf("\033[%sm%snode: (%s)[%s][lc:%u][p%u:c%u:l%u:r%u]\033[0m\n", node->Head.Color==RBTREE_NODECOLOR_RED?"31":"34", isRight?"r":"l", KeySerialization<KeyT>::Serialization(node->Key).c_str(), node->Head.Color?"black":"red", node->Head.LeftCount, node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
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
				printf("\033[%smroot: (%s)[%s][lc:%u][p%u:c%u:l%u:r%u]\033[0m\n", node->Head.Color==RBTREE_NODECOLOR_RED?"31":"34", KeySerialization<KeyT>::Serialization(node->Key).c_str(), node->Head.Color?"black":"red", node->Head.LeftCount, node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
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
		RBTreeNode<KeyT, ValueT, IndexT>* leftNode = m_NodeBlockTable[node->Head.LeftIndex];
		PrintNode(leftNode, layer+1, false, flags);

		RBTreeNode<KeyT, ValueT, IndexT>* rightNode = m_NodeBlockTable[node->Head.RightIndex];
		PrintNode(rightNode, layer+1, true, flags);
	}

	BlockTable<RBTreeNode<KeyT, ValueT, IndexT>, RBTreeHead<IndexT>, IndexT> m_NodeBlockTable;
};

#endif // define __RBTREE_HPP__
