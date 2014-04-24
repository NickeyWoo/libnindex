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

template<typename T>
struct CountAddition
{
	typedef T AdditionType;
};

template<typename T>
struct SumAddition
{
	typedef T AdditionType;
};

template<typename IndexT>
struct RBTreeNodeHead
{
	uint8_t Color;
	IndexT ParentIndex;
	IndexT LeftIndex;
	IndexT RightIndex;
} __attribute__((packed));

template<typename KeyT, typename ValueT, typename AdditionListT, typename IndexT>
struct RBTreeNode;
template<typename KeyT, typename ValueT, typename IndexT>
struct RBTreeNode<KeyT, ValueT, NullType, IndexT>
{
	RBTreeNodeHead<IndexT> Head;
	KeyT Key;
	ValueT Value;
} __attribute__((packed));
template<typename KeyT, typename ValueT, typename CountAdditionType, typename AdditionListT, typename IndexT>
struct RBTreeNode<KeyT, ValueT, TypeList<CountAddition<CountAdditionType>, AdditionListT>, IndexT> :
	public RBTreeNode<KeyT, ValueT, AdditionListT, IndexT>
{
	CountAdditionType LeftCount;
} __attribute__((packed));
template<typename KeyT, typename ValueT, typename SumAdditionType, typename AdditionListT, typename IndexT>
struct RBTreeNode<KeyT, ValueT, TypeList<SumAddition<SumAdditionType>, AdditionListT>, IndexT> :
	public RBTreeNode<KeyT, ValueT, AdditionListT, IndexT>
{
	SumAdditionType LeftSum;
	SumAdditionType SumAdditionValue;
} __attribute__((packed));

template<typename RBTreeNodeT, typename AdditionListT>
struct TreeNodeAddition;
template<typename RBTreeNodeT>
struct TreeNodeAddition<RBTreeNodeT, NullType>
{
	inline static void InsertFixup(RBTreeNodeT* pNode, RBTreeNodeT* pInsertNode)
	{
	}

	inline static void ClearFixup(RBTreeNodeT* pNode, RBTreeNodeT* pClearNode)
	{
	}

	inline static void RotateNodeInitialize(RBTreeNodeT* pNode)
	{
	}

	inline static void RotateFixup(RBTreeNodeT* pNode, RBTreeNodeT* pRightNode)
	{
	}

	inline static std::string Serialization(RBTreeNodeT* pNode)
	{
		return std::string();
	}
};
template<typename RBTreeNodeT, typename CountAdditionType, typename AdditionListT>
struct TreeNodeAddition<RBTreeNodeT, TypeList<CountAddition<CountAdditionType>, AdditionListT> >
{
	inline static void InsertFixup(RBTreeNodeT* pNode, RBTreeNodeT* pInsertNode)
	{
		++pNode->LeftCount;
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::InsertFixup(pNode, pInsertNode);
	}

	inline static void ClearFixup(RBTreeNodeT* pNode, RBTreeNodeT* pClearNode)
	{
		--pNode->LeftCount;
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::ClearFixup(pNode, pClearNode);
	}

	inline static void RotateNodeInitialize(RBTreeNodeT* pFixNode)
	{
		pFixNode->LeftCount = 0;
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::RotateNodeInitialize(pFixNode);
	}

	inline static void RotateFixup(RBTreeNodeT* pFixNode, RBTreeNodeT* pLeftChildRightNode)
	{
		pFixNode->LeftCount += pLeftChildRightNode->LeftCount + 1;
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::RotateFixup(pFixNode, pLeftChildRightNode);
	}

	inline static std::string Serialization(RBTreeNodeT* pNode)
	{
		std::string str = TreeNodeAddition<RBTreeNodeT, AdditionListT>::Serialization(pNode);
		str.append((boost::format("[lc:%s]") % KeySerialization<CountAdditionType>::Serialization(pNode->LeftCount)).str());
		return str;
	}
};
template<typename RBTreeNodeT, typename SumAdditionType, typename AdditionListT>
struct TreeNodeAddition<RBTreeNodeT, TypeList<SumAddition<SumAdditionType>, AdditionListT> >
{
	inline static void InsertFixup(RBTreeNodeT* pNode, RBTreeNodeT* pInsertNode)
	{
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::InsertFixup(pNode, pInsertNode);
	}

	inline static void ClearFixup(RBTreeNodeT* pNode, RBTreeNodeT* pClearNode)
	{
		pNode->LeftSum -= pClearNode->SumAdditionValue;
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::ClearFixup(pNode, pClearNode);
	}

	inline static void RotateNodeInitialize(RBTreeNodeT* pFixNode)
	{
		pFixNode->LeftSum = 0;
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::RotateNodeInitialize(pFixNode);
	}

	inline static void RotateFixup(RBTreeNodeT* pFixNode, RBTreeNodeT* pLeftChildRightNode)
	{
		pFixNode->LeftSum += pLeftChildRightNode->LeftSum + pLeftChildRightNode->SumAdditionValue;
		TreeNodeAddition<RBTreeNodeT, AdditionListT>::RotateFixup(pFixNode, pLeftChildRightNode);
	}

	inline static std::string Serialization(RBTreeNodeT* pNode)
	{
		std::string str = TreeNodeAddition<RBTreeNodeT, AdditionListT>::Serialization(pNode);
		str.append((boost::format("[ls:%s,av:%s]") % KeySerialization<SumAdditionType>::Serialization(pNode->LeftSum)
													% KeySerialization<SumAdditionType>::Serialization(pNode->SumAdditionValue)).str());
		return str;
	}
};

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

template<typename KeyT, typename ValueT, typename AdditionListT, typename IndexT>
class RBTree;

template<typename KeyT, typename ValueT, typename AdditionListT, typename IndexT, typename RBTreeNodeT>
class RBTreeImpl;
template<typename KeyT, typename ValueT, typename IndexT, typename AdditionListT>
class RBTreeImpl<KeyT, ValueT, NullType, IndexT, RBTreeNode<KeyT, ValueT, AdditionListT, IndexT> >
{
public:
	typedef RBTreeIteratorImpl<IndexT> RBTreeIterator;
	typedef RBTreeNode<KeyT, ValueT, AdditionListT, IndexT> RBTreeNodeType;
	typedef RBTree<KeyT, ValueT, AdditionListT, IndexT> RBTreeType;

	static RBTreeType CreateRBTree(IndexT size)
	{
		RBTreeType rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNodeType, RBTreeHead<IndexT>, IndexT>::CreateBlockTable(size);
		return rbt;
	}

	static RBTreeType LoadRBTree(char* buffer, size_t size)
	{
		RBTreeType rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNodeType, RBTreeHead<IndexT>, IndexT>::LoadBlockTable(buffer, size);
		return rbt;
	}

	template<typename StorageT>
	static RBTreeType LoadRBTree(StorageT storage)
	{
		RBTreeType rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNodeType, RBTreeHead<IndexT>, IndexT>::LoadBlockTable(storage);
		return rbt;
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return BlockTable<RBTreeNodeType, RBTreeHead<IndexT>, IndexT>::GetBufferSize(size);
	}

	void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	void Clear(KeyT key)
	{
		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNodeType* node = m_NodeBlockTable[pHead->RootIndex];
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
		RBTreeNodeType* node = TreeNodeMinimum(m_NodeBlockTable[pHead->RootIndex]);
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
		RBTreeNodeType* node = TreeNodeMaximum(m_NodeBlockTable[pHead->RootIndex]);
		if(node)
		{
			if(pKey)
				memcpy(pKey, &node->Key, sizeof(KeyT));
			return &node->Value;
		}
		return NULL;
	}

	RBTreeIterator Iterator()
	{
		RBTreeIterator iter;

		RBTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNodeType* node = TreeNodeMinimum(m_NodeBlockTable[pHead->RootIndex]);
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

		RBTreeNodeType* node = m_NodeBlockTable[pHead->RootIndex];
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
					RBTreeNodeType* pParentNode = NULL;
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
		RBTreeNodeType* pNode = m_NodeBlockTable[pIter->Index];
		if(pNode)
		{
			RBTreeNodeType* pRightNode = m_NodeBlockTable[pNode->Head.RightIndex];
			if(pRightNode == NULL)
			{
				IndexT currentNodeIndex = pIter->Index;
				RBTreeNodeType* pCurrentNode = pNode;
				RBTreeNodeType* pParentNode = NULL;
				while((pParentNode = m_NodeBlockTable[pCurrentNode->Head.ParentIndex]) && pParentNode->Head.RightIndex == currentNodeIndex)
				{
					currentNodeIndex = pCurrentNode->Head.ParentIndex;
					pCurrentNode = pParentNode;
				}
				pIter->Index = pCurrentNode->Head.ParentIndex;
			}
			else
			{
				RBTreeNodeType* pMiniNode = TreeNodeMinimum(pRightNode);
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
		RBTreeNodeType* node = m_NodeBlockTable[pHead->RootIndex];

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

			node->Head.ParentIndex = ParentIdx;
			memcpy(&node->Key, &key, sizeof(KeyT));

			InsertAdditionFixup(*pEmptyIdx, node);
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
		RBTreeNodeType* node = m_NodeBlockTable[pHead->RootIndex];
		PrintNode(node, 0, false, flags);
	}

protected:
	inline RBTreeNodeType* TreeNodeMaximum(RBTreeNodeType* pNode)
	{
		while(pNode && m_NodeBlockTable[pNode->Head.RightIndex])
			pNode = m_NodeBlockTable[pNode->Head.RightIndex];
		return pNode;
	}

	inline RBTreeNodeType* TreeNodeMinimum(RBTreeNodeType* pNode)
	{
		while(pNode && m_NodeBlockTable[pNode->Head.LeftIndex])
			pNode = m_NodeBlockTable[pNode->Head.LeftIndex];
		return pNode;
	}

	void TreeNodeTransplantRight(RBTreeNodeType* pNode, IndexT nodeIndex, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || m_NodeBlockTable[pNode->Head.LeftIndex] || !pHead)
			return;

		RBTreeNodeType* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		RBTreeNodeType* pRightNode = m_NodeBlockTable[pNode->Head.RightIndex];

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

	void TreeNodeTransplantLeft(RBTreeNodeType* pNode, IndexT nodeIndex, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || m_NodeBlockTable[pNode->Head.RightIndex] || !pHead)
			return;

		RBTreeNodeType* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
		RBTreeNodeType* pLeftNode = m_NodeBlockTable[pNode->Head.LeftIndex];

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

	void ClearOneChildNode(RBTreeNodeType* pNode, RBTreeHead<IndexT>* pHead)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);
		ClearAdditionFixup(nodeIndex, pNode);

		bool needFixup = (pNode->Head.Color == RBTREE_NODECOLOR_BLACK);

		RBTreeNodeType* pFixNode = NULL;
		RBTreeNodeType* pFixParentNode = NULL;

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

	void ClearNode(RBTreeNodeType* pNode, RBTreeHead<IndexT>* pHead)
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
			RBTreeNodeType* pMiniNode = TreeNodeMinimum(m_NodeBlockTable[pNode->Head.RightIndex]);

			// clone Key and Data, then Clear MiniNode.
			memcpy(&pNode->Key, &pMiniNode->Key, sizeof(KeyT));
			memcpy(&pNode->Value, &pMiniNode->Value, sizeof(ValueT));

			ClearOneChildNode(pMiniNode, pHead);
		}
	}

	void ClearFixup(RBTreeNodeType* pNode, RBTreeNodeType* pParentNode, RBTreeHead<IndexT>* pHead)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);
		while((!pNode || pNode->Head.Color == RBTREE_NODECOLOR_BLACK) && nodeIndex != pHead->RootIndex)
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
			{
				RBTreeNodeType* pBrotherNode = m_NodeBlockTable[pParentNode->Head.RightIndex];
				if(pBrotherNode->Head.Color == RBTREE_NODECOLOR_RED)
				{
					pBrotherNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pParentNode->Head.Color = RBTREE_NODECOLOR_RED;
					TreeNodeRotateLeft(pParentNode, pHead);

					pBrotherNode = m_NodeBlockTable[pParentNode->Head.RightIndex];
				}

				RBTreeNodeType* pBrotherLeftNode = m_NodeBlockTable[pBrotherNode->Head.LeftIndex];
				RBTreeNodeType* pBrotherRightNode = m_NodeBlockTable[pBrotherNode->Head.RightIndex];
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
				RBTreeNodeType* pBrotherNode = m_NodeBlockTable[pParentNode->Head.LeftIndex];
				if(pBrotherNode->Head.Color == RBTREE_NODECOLOR_RED)
				{
					pBrotherNode->Head.Color = RBTREE_NODECOLOR_BLACK;
					pParentNode->Head.Color = RBTREE_NODECOLOR_RED;
					TreeNodeRotateRight(pParentNode, pHead);

					pBrotherNode = m_NodeBlockTable[pParentNode->Head.LeftIndex];
				}

				RBTreeNodeType* pBrotherLeftNode = m_NodeBlockTable[pBrotherNode->Head.LeftIndex];
				RBTreeNodeType* pBrotherRightNode = m_NodeBlockTable[pBrotherNode->Head.RightIndex];
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

	void InsertFixup(RBTreeNodeType* pNode, RBTreeHead<IndexT>* pHead)
	{
		RBTreeNodeType* pParentNode = NULL;
		while(pNode && 
			  pNode->Head.Color == RBTREE_NODECOLOR_RED &&
			  (pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex]) &&
			  pParentNode->Head.Color == RBTREE_NODECOLOR_RED)
		{
			IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

			RBTreeNodeType* pGrandpaNode = m_NodeBlockTable[pParentNode->Head.ParentIndex];
			if(pGrandpaNode->Head.LeftIndex == pNode->Head.ParentIndex)
			{
				RBTreeNodeType* pUncleNode = m_NodeBlockTable[pGrandpaNode->Head.RightIndex];
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
				RBTreeNodeType* pUncleNode = m_NodeBlockTable[pGrandpaNode->Head.LeftIndex];
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

	void TreeNodeRotateLeft(RBTreeNodeType* pNode, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || !pHead)
			return;

		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

		RBTreeNodeType* pRightNode = m_NodeBlockTable[pNode->Head.RightIndex];
		if(!pRightNode)
			return;
		pRightNode->Head.ParentIndex = pNode->Head.ParentIndex;

		RBTreeNodeType* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
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

		RBTreeNodeType* pRightLeftNode = m_NodeBlockTable[pRightNode->Head.LeftIndex];
		if(pRightLeftNode)
			pRightLeftNode->Head.ParentIndex = nodeIndex;

		pRightNode->Head.LeftIndex = nodeIndex;

		RotateAdditionFixup(pRightNode);
	}

	void TreeNodeRotateRight(RBTreeNodeType* pNode, RBTreeHead<IndexT>* pHead)
	{
		if(!pNode || !pHead)
			return;

		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

		RBTreeNodeType* pLeftNode = m_NodeBlockTable[pNode->Head.LeftIndex];
		if(!pLeftNode)
			return;
		pLeftNode->Head.ParentIndex = pNode->Head.ParentIndex;

		RBTreeNodeType* pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex];
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

		RBTreeNodeType* pLeftRightNode = m_NodeBlockTable[pLeftNode->Head.RightIndex];
		if(pLeftRightNode)
			pLeftRightNode->Head.ParentIndex = nodeIndex;

		pLeftNode->Head.RightIndex = nodeIndex;

		RotateAdditionFixup(pNode);
	}

	void PrintNode(RBTreeNodeType* node, std::vector<bool>::size_type layer, bool isRight, std::vector<bool>& flags)
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
				printf("\033[%sm%snode: (%s)[%s]%s[p%u:c%u:l%u:r%u]\033[0m\n", node->Head.Color==RBTREE_NODECOLOR_RED?"31":"34", isRight?"r":"l", KeySerialization<KeyT>::Serialization(node->Key).c_str(), node->Head.Color?"black":"red", TreeNodeAddition<RBTreeNodeType, AdditionListT>::Serialization(node).c_str(), node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
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
				printf("\033[%smroot: (%s)[%s]%s[p%u:c%u:l%u:r%u]\033[0m\n", node->Head.Color==RBTREE_NODECOLOR_RED?"31":"34", KeySerialization<KeyT>::Serialization(node->Key).c_str(), node->Head.Color?"black":"red", TreeNodeAddition<RBTreeNodeType, AdditionListT>::Serialization(node).c_str(), node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
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
		RBTreeNodeType* leftNode = m_NodeBlockTable[node->Head.LeftIndex];
		PrintNode(leftNode, layer+1, false, flags);

		RBTreeNodeType* rightNode = m_NodeBlockTable[node->Head.RightIndex];
		PrintNode(rightNode, layer+1, true, flags);
	}

	void InsertAdditionFixup(IndexT nodeIndex, RBTreeNodeType* pNode)
	{
		RBTreeNodeType* pInsertNode = pNode;
		RBTreeNodeType* pParentNode = NULL;
		while(pNode && (pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex]))
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				TreeNodeAddition<RBTreeNodeType, AdditionListT>::InsertFixup(pParentNode, pInsertNode);

			nodeIndex = pNode->Head.ParentIndex;
			pNode = pParentNode;
		}
	}

	void ClearAdditionFixup(IndexT nodeIndex, RBTreeNodeType* pNode)
	{
		RBTreeNodeType* pClearNode = pNode;
		RBTreeNodeType* pParentNode = NULL;
		while(pNode && (pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex]))
		{
			if(pParentNode->Head.LeftIndex == nodeIndex)
				TreeNodeAddition<RBTreeNodeType, AdditionListT>::ClearFixup(pParentNode, pClearNode);

			nodeIndex = pNode->Head.ParentIndex;
			pNode = pParentNode;
		}
	}

	void RotateAdditionFixup(RBTreeNodeType* pNode)
	{
		if(!pNode)
			return;

		TreeNodeAddition<RBTreeNodeType, AdditionListT>::RotateNodeInitialize(pNode);
		RBTreeNodeType* pTempNode = m_NodeBlockTable[pNode->Head.LeftIndex];
		while(pTempNode)
		{
			TreeNodeAddition<RBTreeNodeType, AdditionListT>::RotateFixup(pNode, pTempNode);
			pTempNode = m_NodeBlockTable[pTempNode->Head.RightIndex];
		}
	}

	BlockTable<RBTreeNodeType, RBTreeHead<IndexT>, IndexT> m_NodeBlockTable;
};
template<typename KeyT, typename ValueT, typename CountAdditionType, typename AdditionListT, typename IndexT, typename RBTreeNodeT>
class RBTreeImpl<KeyT, ValueT, TypeList<CountAddition<CountAdditionType>, AdditionListT>, IndexT, RBTreeNodeT> :
	public RBTreeImpl<KeyT, ValueT, AdditionListT, IndexT, RBTreeNodeT>
{
public:
	CountAdditionType Count(typename RBTree<KeyT, ValueT, AdditionListT, IndexT>::RBTreeIterator iter)
	{
		CountAdditionType count = 0;

		IndexT nodeIndex = iter.Index;
		RBTreeNodeT* node = this->m_NodeBlockTable[nodeIndex];
		while(node != NULL)
		{
			count += node->LeftCount + 1;

			RBTreeNodeT* parentNode = NULL;
			while((parentNode = this->m_NodeBlockTable[node->Head.ParentIndex]) && parentNode->Head.LeftIndex == nodeIndex)
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

	CountAdditionType Count(typename RBTree<KeyT, ValueT, AdditionListT, IndexT>::RBTreeIterator iterBegin, 
							typename RBTree<KeyT, ValueT, AdditionListT, IndexT>::RBTreeIterator iterEnd)
	{
		CountAdditionType c1 = Count(iterBegin);
		CountAdditionType c2 = Count(iterEnd);
		if(c1 > c2)
			return 0;
		else
			return c2 - c1;
	}

};
template<typename KeyT, typename ValueT, typename SumAdditionType, typename AdditionListT, typename IndexT, typename RBTreeNodeT>
class RBTreeImpl<KeyT, ValueT, TypeList<SumAddition<SumAdditionType>, AdditionListT>, IndexT, RBTreeNodeT> :
	public RBTreeImpl<KeyT, ValueT, AdditionListT, IndexT, RBTreeNodeT>
{
public:

	void GetAddition(KeyT key, SumAdditionType* pValue)
	{
		RBTreeHead<IndexT>* pHead = this->m_NodeBlockTable.GetHead();
		RBTreeNodeT* pNode = this->m_NodeBlockTable[pHead->RootIndex];
		while(pNode != NULL)
		{
			int result = KeyCompare<KeyT>::Compare(pNode->Key, key);
			if(result == 0)
				break;
			else if(result > 0)
				pNode = this->m_NodeBlockTable[pNode->Head.LeftIndex];
			else
				pNode = this->m_NodeBlockTable[pNode->Head.RightIndex];
		}
		if(pNode)
			memcpy(pValue, &pNode->SumAdditionValue, sizeof(SumAdditionType));
		else
			memset(pValue, 0, sizeof(SumAdditionType));
	}

	void SetAddition(KeyT key, SumAdditionType value)
	{
		RBTreeHead<IndexT>* pHead = this->m_NodeBlockTable.GetHead();
		RBTreeNodeT* pNode = this->m_NodeBlockTable[pHead->RootIndex];
		while(pNode != NULL)
		{
			int result = KeyCompare<KeyT>::Compare(pNode->Key, key);
			if(result == 0)
				break;
			else if(result > 0)
				pNode = this->m_NodeBlockTable[pNode->Head.LeftIndex];
			else
				pNode = this->m_NodeBlockTable[pNode->Head.RightIndex];
		}
		if(pNode)
		{
			pNode->SumAdditionValue = value;

			RBTreeNodeT* pParentNode = NULL;
			while(pNode && (pParentNode = this->m_NodeBlockTable[pNode->Head.ParentIndex]))
			{
				if(pParentNode->Head.LeftIndex == this->m_NodeBlockTable.GetBlockID(pNode))
					pParentNode->LeftSum += value;
				pNode = pParentNode;
			}
		}
	}

	SumAdditionType Sum(typename RBTree<KeyT, ValueT, AdditionListT, IndexT>::RBTreeIterator iter)
	{
		SumAdditionType sum = 0;

		IndexT nodeIndex = iter.Index;
		RBTreeNodeT* node = this->m_NodeBlockTable[nodeIndex];
		while(node != NULL)
		{
			sum += node->LeftSum + node->SumAdditionValue;

			RBTreeNodeT* parentNode = NULL;
			while((parentNode = this->m_NodeBlockTable[node->Head.ParentIndex]) && parentNode->Head.LeftIndex == nodeIndex)
			{
				nodeIndex = node->Head.ParentIndex;
				node = parentNode;
			}

			if(parentNode == NULL)
				break;

			nodeIndex = node->Head.ParentIndex;
			node = parentNode;
		}
		return sum;
	}

	SumAdditionType Sum(typename RBTree<KeyT, ValueT, AdditionListT, IndexT>::RBTreeIterator iterBegin, 
						typename RBTree<KeyT, ValueT, AdditionListT, IndexT>::RBTreeIterator iterEnd)
	{
		SumAdditionType s1 = Sum(iterBegin);
		SumAdditionType s2 = Sum(iterEnd);
		if(s1 > s2)
			return 0;
		else
			return s2 - s1;
	}

};

template<typename KeyT, typename ValueT, 
	typename AdditionListT = NullType, typename IndexT = uint32_t>
class RBTree :
	public RBTreeImpl<KeyT, ValueT, AdditionListT, IndexT, RBTreeNode<KeyT, ValueT, AdditionListT, IndexT> >
{
};


#endif // define __RBTREE_HPP__
