/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2014.01.10
 *
*--*/
#ifndef __KDTREE_HPP__
#define __KDTREE_HPP__

#include <utility>
#include <vector>
#include <algorithm>
#include "keyutility.hpp"
#include "blocktable.hpp"
#include "heap.hpp"
#include "distance.hpp"

template<typename KeyT, uint8_t Dimensions>
struct NodeVector
{
	KeyT Key[Dimensions];

	inline bool operator==(NodeVector<KeyT, Dimensions>& key)
	{
		return !(operator==(key));
	}

	inline bool operator!=(NodeVector<KeyT, Dimensions>& key)
	{
		for(uint8_t i=0; i<Dimensions; ++i)
			if(Key[i] != key.Key[i])
				return true;
		return false;
	}

	inline KeyT& operator[](uint8_t index)
	{
		return Key[index];
	}

} __attribute__((packed));

template<typename IndexT>
struct KDNodeHead
{
	uint8_t SplitDimensions;

	IndexT LeftIndex;
	IndexT RightIndex;
	IndexT ParentIndex;
} __attribute__((packed));

template<typename ValueT, uint8_t Dimensions, typename KeyT, typename IndexT>
struct KDNode
{
	KDNodeHead<IndexT> Head;

	NodeVector<KeyT, Dimensions> Vector;
	ValueT Value;
} __attribute__((packed));

template<typename IndexT>
struct KDTreeHead
{
	IndexT RootIndex;
} __attribute__((packed));

template<typename ValueT, uint8_t Dimensions, 
			typename KeyT = uint32_t, template<typename, uint8_t> class DistanceT = EuclideanDistance, 
			typename IndexT = uint32_t>
class KDTree
{
public:
	typedef NodeVector<KeyT, Dimensions> VectorType;

	typedef struct {
		VectorType Vector;
		ValueT Value;
	} DataType;

	static KDTree<ValueT, Dimensions, KeyT, DistanceT, IndexT> CreateKDTree(IndexT size)
	{
		KDTree<ValueT, Dimensions, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, Dimensions, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::CreateBlockTable(size);
		return kdtree;
	}

	static KDTree<ValueT, Dimensions, KeyT, DistanceT, IndexT> LoadKDTree(char* buffer, size_t size)
	{
		KDTree<ValueT, Dimensions, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, Dimensions, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::LoadBlockTable(buffer, size);
		return kdtree;
	}

	template<typename StorageT>
	static KDTree<ValueT, Dimensions, KeyT, DistanceT, IndexT> LoadKDTree(StorageT storage)
	{
		KDTree<ValueT, Dimensions, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, Dimensions, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::LoadBlockTable(storage);
		return kdtree;
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return BlockTable<KDNode<ValueT, Dimensions, KeyT, IndexT>, 
							KDTreeHead<IndexT>, IndexT>::GetBufferSize(size);
	}

	void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	int Build(std::vector<DataType>& datas)
	{

		return 0;
	}

	inline ValueT* Nearest(VectorType key)
	{
		return NULL;
	}

	int Nearest(VectorType key, ValueT** buffer, size_t size)
	{
		return NULL;
	}

	int Range(VectorType key, KeyT range, ValueT** buffer, size_t size)
	{
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

		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		KDNode<ValueT, Dimensions, KeyT, IndexT>* node = m_NodeBlockTable[pHead->RootIndex];
		PrintNode(node, 0, false, flags);
	}

	KDTree()
	{
	}

protected:

	inline bool IsLeaf(KDNode<ValueT, Dimensions, KeyT, IndexT>* pNode)
	{
		return (m_NodeBlockTable[pNode->Head.LeftIndex] == NULL && m_NodeBlockTable[pNode->Head.RightIndex] == NULL);
	}

	std::string NodeSerialization(KDNode<ValueT, Dimensions, KeyT, IndexT>* pNode)
	{
		std::string str;
		if(IsLeaf(pNode))
		{
			// leaf node
			str.append("(");
			for(uint8_t i=0; i<Dimensions; ++i)
				str.append((boost::format("%s, ") % KeySerialization<KeyT>::Serialization(pNode->Vector[i])).str());
			str.erase(str.length() - 2);
			str.append(")");
		}
		else
		{
			// inner node
			str.append((boost::format("[split:%hhu, value:%s]") % pNode->Head.SplitDimensions % KeySerialization<KeyT>::Serialization(pNode->Vector[pNode->Head.SplitDimensions])).str());
		}
		return str;
	}

	void PrintNode(KDNode<ValueT, Dimensions, KeyT, IndexT>* node, std::vector<bool>::size_type layer, bool isRight, std::vector<bool>& flags)
	{
		IndexT nodeIndex = m_NodeBlockTable.GetBlockID(node);
		if(layer > 0)
		{
			for(uint32_t c=0; c<layer; ++c)
			{
				if(flags.at(c))
					printf("|    ");
				else
					printf("     ");
			}
			if(isRight)
				printf("\\-");
			else
				printf("|-");
			if(node)
				printf("%snode: %s[p%u:c%u:l%u:r%u]\n", isRight?"r":"l", NodeSerialization(node).c_str(), node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
			else
			{
				printf("%snode: (null)\n", isRight?"r":"l");
				return;
			}
		}
		else
		{
			printf("\\-");
			if(node)
				printf("root: %s[p%u:c%u:l%u:r%u]\n", NodeSerialization(node).c_str(), node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
			else
			{
				printf("root: (null)\n");
				return;
			}
		}
	
		if(isRight)
			flags.at(layer) = false;
		else if(layer > 0)
			flags.at(layer) = true;

		flags.push_back(true);
		KDNode<ValueT, Dimensions, KeyT, IndexT>* leftNode = m_NodeBlockTable[node->Head.LeftIndex];
		PrintNode(leftNode, layer+1, false, flags);

		KDNode<ValueT, Dimensions, KeyT, IndexT>* rightNode = m_NodeBlockTable[node->Head.RightIndex];
		PrintNode(rightNode, layer+1, true, flags);
	}

	BlockTable<KDNode<ValueT, Dimensions, KeyT, IndexT>, 
				KDTreeHead<IndexT>, IndexT> m_NodeBlockTable;
};

#endif // define __KDTREE_HPP__
