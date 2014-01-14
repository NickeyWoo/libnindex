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
		NodeVector<KeyT, Dimensions> Vector;
		ValueT Value;
	} ImportDataType;

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

	int OptimumImport(ImportDataType* pBuffer, size_t size)
	{
		if(!pBuffer || size == 0)
			return -1;

		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		KDNode<ValueT, Dimensions, KeyT, IndexT>* pNode = m_NodeBlockTable[pHead->RootIndex];
		if(pNode)
			return -1;

		ImportNodes(&pHead->RootIndex, NULL, pBuffer, size);
		return 0;
	}

	inline ValueT* Nearest(VectorType key)
	{
		return Nearest(key, 1);
	}

	ValueT* Nearest(VectorType key, size_t k)
	{
		return NULL;
	}

	ValueT* Range(VectorType key, KeyT range, ValueT* buffer, size_t size)
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

	void ImportNodes(IndexT* pIndex, KDNode<ValueT, Dimensions, KeyT, IndexT>* pParentNode, ImportDataType* pBuffer, size_t size)
	{
		if(!pIndex || !pBuffer || size == 0)
			return;

		*pIndex = m_NodeBlockTable.AllocateBlock();
		KDNode<ValueT, Dimensions, KeyT, IndexT>* pNode = m_NodeBlockTable[*pIndex];
		if(!pNode)
			return;

		memset(pNode, 0, sizeof(KDNode<ValueT, Dimensions, KeyT, IndexT>));

		pNode->Head.ParentIndex = m_NodeBlockTable.GetBlockID(pParentNode);
		if(pParentNode)
			pNode->Head.SplitDimensions = (pParentNode->Head.SplitDimensions + 1) % Dimensions;
		else
			pNode->Head.SplitDimensions = 0;

		ImportDataType* pMedianData = GetMedianData(pBuffer, size, pNode->Head.SplitDimensions);
		memcpy(&pNode->Vector, &pMedianData->Vector, sizeof(VectorType));
		memcpy(&pNode->Value, &pMedianData->Value, sizeof(ValueT));

		size_t leftSize = size / 2;
		ImportNodes(&pNode->Head.LeftIndex, pNode, pBuffer, leftSize);
		ImportNodes(&pNode->Head.RightIndex, pNode, &pBuffer[leftSize + 1], size - leftSize - 1);
	}

	class DataCompare
	{
	public:
		DataCompare(uint8_t dim) :
			m_Dimensions(dim)
		{
		}

		inline bool operator()(const ImportDataType& d1, const ImportDataType& d2) const
		{
			return d1.Vector.Key[m_Dimensions] < d2.Vector.Key[m_Dimensions];
		}

	private:
		uint8_t m_Dimensions;
	};

	ImportDataType* GetMedianData(ImportDataType* pBuffer, size_t size, uint8_t dim)
	{
		if(!pBuffer)
			return NULL;

		DataCompare cmp(dim);
		std::sort(&pBuffer[0], &pBuffer[size], cmp);
		return &pBuffer[size/2];
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
				printf("%snode: (%s)[%hhu][p%u:c%u:l%u:r%u]\n", isRight?"r":"l", VectorSerialization<KeyT, Dimensions>::Serialization(&node->Vector).c_str(), node->Head.SplitDimensions, node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
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
				printf("root: (%s)[%hhu][p%u:c%u:l%u:r%u]\n", VectorSerialization<KeyT, Dimensions>::Serialization(&node->Vector).c_str(), node->Head.SplitDimensions, node->Head.ParentIndex, nodeIndex, node->Head.LeftIndex, node->Head.RightIndex);
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
