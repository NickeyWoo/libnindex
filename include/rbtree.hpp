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

struct RBTreeNodeHead
{
	uint8_t Color;
	uint32_t ParentIndex;
	uint32_t LeftIndex;
	uint32_t RightIndex;
} __attribute__((packed));

template<typename KeyT, typename ValueT>
struct RBTreeNode
{
	RBTreeNodeHead Head;
	KeyT Key;
	ValueT Value;
} __attribute__((packed));

#define RBTREE_MAGIC    "RBTREE@@"
#define RBTREE_VERSION  0x0101

template<typename HeadT>
struct RBTreeHead
{
    char cMagic[8];
    uint16_t wVersion;
	uint32_t RootIndex;
    uint32_t dwReserved[4];

	HeadT Head;
} __attribute__((packed));
template<>
struct RBTreeHead<void>
{
    char cMagic[8];
    uint16_t wVersion;
	uint32_t RootIndex;
    uint32_t dwReserved[4];
} __attribute__((packed));

struct RBTreeIteratorImpl
{
	uint32_t Index;

	bool operator == (const RBTreeIteratorImpl& iter)
	{
		return (Index == iter.Index);
	}

	bool operator != (const RBTreeIteratorImpl& iter)
	{
		return (Index != iter.Index);
	}
};

template<typename KeyT, typename ValueT, typename HeadT = void>
class RBTree
{
public:
	typedef RBTreeIteratorImpl RBTreeIterator;
	typedef RBTreeNode<KeyT, ValueT> RBTreeNodeType;
	typedef RBTree<KeyT, ValueT, HeadT> RBTreeType;

	static RBTreeType CreateRBTree(uint32_t size)
	{
		RBTreeType rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNodeType, RBTreeHead<HeadT> >::CreateBlockTable(size);

        RBTreeHead<HeadT>* pstHead = rbt.m_NodeBlockTable.GetHead();
        if(pstHead)
        {
            memcpy(pstHead->cMagic, RBTREE_MAGIC, 8);
            pstHead->wVersion = RBTREE_VERSION;
            pstHead->RootIndex = 0;
        }
		return rbt;
	}

	static RBTreeType LoadRBTree(char* buffer, size_t size)
	{
		RBTreeType rbt;
		rbt.m_NodeBlockTable = BlockTable<RBTreeNodeType, RBTreeHead<HeadT> >::LoadBlockTable(buffer, size);

        RBTreeHead<HeadT>* pstHead = rbt.m_NodeBlockTable.GetHead();
        if(pstHead)
        {
            if(memcmp(pstHead->cMagic, "\0\0\0\0\0\0\0\0", 8) == 0)
            {
                memcpy(pstHead->cMagic, RBTREE_MAGIC, 8);
                pstHead->wVersion = RBTREE_VERSION;
                pstHead->RootIndex = 0;
            }
            else
            {
                if(memcmp(pstHead->cMagic, RBTREE_MAGIC, 8) != 0 ||
                    pstHead->wVersion != RBTREE_VERSION)
                {
                    rbt.m_NodeBlockTable.Delete();
                }
            }
        }
		return rbt;
	}

	template<typename StorageT>
	static RBTreeType LoadRBTree(StorageT storage)
	{
		return RBTree<KeyT, ValueT, HeadT>::LoadRBTree(storage.GetStorageBuffer(), storage.GetSize());
	}

	static inline size_t GetBufferSize(uint32_t size)
	{
		return BlockTable<RBTreeNodeType, RBTreeHead<HeadT> >::GetBufferSize(size);
	}

    inline bool Success()
    {
        return m_NodeBlockTable.Success();
    }

    inline float Capacity()
    {
        return m_NodeBlockTable.Capacity();
    }

	void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	void Clear(KeyT key)
	{
		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();
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
		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();
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
		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();
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

		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNodeType* node = TreeNodeMinimum(m_NodeBlockTable[pHead->RootIndex]);
		if(node)
			iter.Index = m_NodeBlockTable.GetBlockID(node);
		else
			iter.Index = pHead->RootIndex;
		return iter;
	}

	RBTreeIterator Iterator(KeyT key)
	{
		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();

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
				uint32_t currentNodeIndex = pIter->Index;
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
		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();
		uint32_t CurIdx = pHead->RootIndex;
		uint32_t ParentIdx = pHead->RootIndex;

		uint32_t* pEmptyIdx = &pHead->RootIndex;
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

		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();
		RBTreeNodeType* node = m_NodeBlockTable[pHead->RootIndex];
		PrintNode(node, 0, false, flags);
	}

	HeadT* GetHead()
	{
		RBTreeHead<HeadT>* pHead = m_NodeBlockTable.GetHead();
		return &pHead->Head;
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

	void TreeNodeTransplantRight(RBTreeNodeType* pNode, uint32_t nodeIndex, RBTreeHead<HeadT>* pHead)
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

	void TreeNodeTransplantLeft(RBTreeNodeType* pNode, uint32_t nodeIndex, RBTreeHead<HeadT>* pHead)
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

	void ClearOneChildNode(RBTreeNodeType* pNode, RBTreeHead<HeadT>* pHead)
	{
		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);
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

	void ClearNode(RBTreeNodeType* pNode, RBTreeHead<HeadT>* pHead)
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

	void ClearFixup(RBTreeNodeType* pNode, RBTreeNodeType* pParentNode, RBTreeHead<HeadT>* pHead)
	{
		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);
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

	void InsertFixup(RBTreeNodeType* pNode, RBTreeHead<HeadT>* pHead)
	{
		RBTreeNodeType* pParentNode = NULL;
		while(pNode && 
			  pNode->Head.Color == RBTREE_NODECOLOR_RED &&
			  (pParentNode = m_NodeBlockTable[pNode->Head.ParentIndex]) &&
			  pParentNode->Head.Color == RBTREE_NODECOLOR_RED)
		{
			uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

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

	void TreeNodeRotateLeft(RBTreeNodeType* pNode, RBTreeHead<HeadT>* pHead)
	{
		if(!pNode || !pHead)
			return;

		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

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
	}

	void TreeNodeRotateRight(RBTreeNodeType* pNode, RBTreeHead<HeadT>* pHead)
	{
		if(!pNode || !pHead)
			return;

		uint32_t nodeIndex = m_NodeBlockTable.GetBlockID(pNode);

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
	}

	void PrintNode(RBTreeNodeType* node, std::vector<bool>::size_type layer, bool isRight, std::vector<bool>& flags)
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
							node->Head.Color==RBTREE_NODECOLOR_RED?"31":"34",
							isRight?"r":"l",
							KeySerialization<KeyT>::Serialization(node->Key).c_str(),
							node->Head.Color?"black":"red",
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
							node->Head.Color==RBTREE_NODECOLOR_RED?"31":"34",
							KeySerialization<KeyT>::Serialization(node->Key).c_str(),
							node->Head.Color?"black":"red",
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
		RBTreeNodeType* leftNode = m_NodeBlockTable[node->Head.LeftIndex];
		PrintNode(leftNode, layer+1, false, flags);

		RBTreeNodeType* rightNode = m_NodeBlockTable[node->Head.RightIndex];
		PrintNode(rightNode, layer+1, true, flags);
	}

	BlockTable<RBTreeNodeType, RBTreeHead<HeadT> > m_NodeBlockTable;
};

#endif // define __RBTREE_HPP__
