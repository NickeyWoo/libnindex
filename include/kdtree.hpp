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

template<typename KeyT, uint8_t DimensionValue>
struct KDVector
{
	KeyT Key[DimensionValue];

	inline bool operator==(KDVector<KeyT, DimensionValue>& key)
	{
		return !(operator!=(key));
	}

	inline bool operator!=(KDVector<KeyT, DimensionValue>& key)
	{
		for(uint8_t i=0; i<DimensionValue; ++i)
			if(Key[i] != key.Key[i])
				return true;
		return false;
	}

	inline bool operator<(KDVector<KeyT, DimensionValue>& key)
	{
		for(uint8_t i=0; i<DimensionValue; ++i)
			if(Key[i] != key.Key[i])
				return (Key[i] < key.Key[i]);
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
	uint8_t SplitDimension;

	IndexT LeftIndex;
	IndexT RightIndex;
	IndexT ParentIndex;
} __attribute__((packed));

template<typename ValueT, uint8_t DimensionValue, typename KeyT, typename IndexT>
struct KDNode
{
	KDNodeHead<IndexT> Head;

	KDVector<KeyT, DimensionValue> Vector;
	ValueT Value;
} __attribute__((packed));

template<typename IndexT>
struct KDTreeHead
{
	IndexT RootIndex;
} __attribute__((packed));

template<typename ValueT, uint8_t DimensionValue, 
			typename KeyT = uint32_t, template<typename, uint8_t> class DistanceT = EuclideanDistance, 
			typename IndexT = uint32_t>
class KDTree
{
public:
	typedef KDVector<KeyT, DimensionValue> VectorType;

	typedef struct {
		VectorType Vector;
		ValueT Value;
	} DataType;

	static KDTree<ValueT, DimensionValue, KeyT, DistanceT, IndexT> CreateKDTree(IndexT size)
	{
		KDTree<ValueT, DimensionValue, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, DimensionValue, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::CreateBlockTable(size);
		return kdtree;
	}

	static KDTree<ValueT, DimensionValue, KeyT, DistanceT, IndexT> LoadKDTree(char* buffer, size_t size)
	{
		KDTree<ValueT, DimensionValue, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, DimensionValue, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::LoadBlockTable(buffer, size);
		return kdtree;
	}

	template<typename StorageT>
	static KDTree<ValueT, DimensionValue, KeyT, DistanceT, IndexT> LoadKDTree(StorageT storage)
	{
		KDTree<ValueT, DimensionValue, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, DimensionValue, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::LoadBlockTable(storage);
		return kdtree;
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return BlockTable<KDNode<ValueT, DimensionValue, KeyT, IndexT>, 
							KDTreeHead<IndexT>, IndexT>::GetBufferSize(size);
	}

	void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	void Clear(VectorType v)
	{
	}

	ValueT* Hash(VectorType vkey, bool isNew = false)
	{
		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode = m_NodeBlockTable[pHead->RootIndex];
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pParentNode = NULL;
		while(pNode != NULL && !IsLeaf(pNode))
		{
			int result = KeyCompare<KeyT>::Compare(vkey[pNode->Head.SplitDimension],
													pNode->Vector[pNode->Head.SplitDimension]);
			if(result > 0)
			{
				pParentNode = pNode;
				pNode = m_NodeBlockTable[pNode->Head.RightIndex];
			}
			else
			{
				pParentNode = pNode;
				pNode = m_NodeBlockTable[pNode->Head.LeftIndex];
			}
		}

		if(pNode && pNode->Vector == vkey)
			return &pNode->Value;
		else if(isNew && pNode)
		{
			IndexT curIndex = m_NodeBlockTable.GetBlockID(pNode);
			IndexT splitIndex = m_NodeBlockTable.AllocateBlock();
			KDNode<ValueT, DimensionValue, KeyT, IndexT>* pSplitNode = m_NodeBlockTable[splitIndex];
			if(!pSplitNode)
				return NULL;

			IndexT newNodeIndex = m_NodeBlockTable.AllocateBlock();
			KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNewNode = m_NodeBlockTable[newNodeIndex];
			if(!pNewNode)
			{
				m_NodeBlockTable.ReleaseBlock(splitIndex);
				return NULL;
			}

			if(pParentNode)
			{
				if(pParentNode->Head.LeftIndex == curIndex)
					pParentNode->Head.LeftIndex = splitIndex;
				else
					pParentNode->Head.RightIndex = splitIndex;
			}
			else
				pHead->RootIndex = splitIndex;

			memset(pSplitNode, 0, sizeof(KDNode<ValueT, DimensionValue, KeyT, IndexT>));
			pSplitNode->Head.ParentIndex = pNode->Head.ParentIndex;
			pSplitNode->Head.SplitDimension = GetMaxVarianceDimension(vkey, pNode->Vector);
			if(vkey[pSplitNode->Head.SplitDimension] > pNode->Vector[pSplitNode->Head.SplitDimension])
			{
				pSplitNode->Head.LeftIndex = curIndex;
				pSplitNode->Head.RightIndex = newNodeIndex;
				pSplitNode->Vector = pNode->Vector;
			}
			else
			{
				pSplitNode->Head.LeftIndex = newNodeIndex;
				pSplitNode->Head.RightIndex = curIndex;
				pSplitNode->Vector = vkey;
			}

			pNode->Head.ParentIndex = splitIndex;

			memset(pNewNode, 0, sizeof(KDNode<ValueT, DimensionValue, KeyT, IndexT>));
			pNewNode->Head.ParentIndex = splitIndex;
			pNewNode->Vector = vkey;
			return &pNewNode->Value;
		}
		else if(isNew && !pNode)
		{
			pHead->RootIndex = m_NodeBlockTable.AllocateBlock();
			KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNewNode = m_NodeBlockTable[pHead->RootIndex];
			if(!pNewNode) return NULL;

			memset(pNewNode, 0, sizeof(KDNode<ValueT, DimensionValue, KeyT, IndexT>));
			pNewNode->Vector = vkey;
			return &pNewNode->Value;
		}
		return NULL;
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
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* node = m_NodeBlockTable[pHead->RootIndex];
		PrintNode(node, 0, false, flags);
	}

	KDTree()
	{
	}

protected:

	uint8_t GetMaxVarianceDimension(VectorType& vkey1, VectorType& vkey2)
	{
		uint8_t dim = 0;
		KeyT max = 0;
		for(uint8_t i=0; i<DimensionValue; ++i)
		{
			KeyT var;
			if(vkey2[i] > vkey1[i]) var = (vkey2[i] - vkey1[i]);
			else var = (vkey1[i] - vkey2[i]);
			if(max < var)
			{
				max = var;
				dim = i;
			}
		}
		return dim;
	}

	uint8_t GetMaxVarianceDimension(std::vector<DataType>& vData)
	{
		uint8_t maxVarianceDimension = 0;

		KeyT maxVariance;
		for(uint8_t i=0; i<DimensionValue; ++i)
		{
			KeyT mean = 0;
			KeyT var = 0;
			for(typename std::vector<DataType>::iterator iter = vData.begin();
				iter != vData.end();
				++iter)
			{
				mean += iter->Vector[i];
			}
			mean /= vData.size();

			for(typename std::vector<DataType>::iterator iter = vData.begin();
				iter != vData.end();
				++iter)
			{
				if(iter->Vector[i] > mean)
					var += (iter->Vector[i] - mean) * (iter->Vector[i] - mean);
				else
					var += (mean - iter->Vector[i]) * (mean - iter->Vector[i]);
			}

			if(var > maxVariance)
			{
				maxVariance = var;
				maxVarianceDimension = i;
			}
		}
		return maxVarianceDimension;
	}

	struct DataCompare
	{
		enum DataCompareType {
			COMPARE_DIMENSION = 0,
			COMPARE_VECTOR = 1,
			COMPARE_EQUAL = 2
		};

		DataCompare(DataCompareType type) :
			m_CompareType(type)
		{
		}

		inline void SetCompareDimension(uint8_t dim)
		{
			m_Dimension = dim;
		}

		inline bool operator()(DataType t1, DataType t2)
		{
			switch(m_CompareType)
			{
			case COMPARE_VECTOR:
				return (t1.Vector < t2.Vector);
			case COMPARE_EQUAL:
				return (t1.Vector == t2.Vector);
			case COMPARE_DIMENSION:
				return (t1.Vector[m_Dimension] < t2.Vector[m_Dimension]);
			}
			return false;
		}

		uint8_t m_Dimension;
		DataCompareType m_CompareType;
	};

	VectorType GetMedianVector(uint8_t dim, std::vector<DataType>& vData)
	{
		typename std::vector<DataType>::size_type median = vData.size() / 2;

		DataCompare compare(DataCompare::COMPARE_DIMENSION);
		compare.SetCompareDimension(dim);
		std::nth_element(vData.begin(), vData.begin()+median, vData.end(), compare);

		return vData.at(median).Value;
	}

	inline bool IsLeaf(KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode)
	{
		return (m_NodeBlockTable[pNode->Head.LeftIndex] == NULL && m_NodeBlockTable[pNode->Head.RightIndex] == NULL);
	}

	std::string NodeSerialization(KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode)
	{
		std::string str;
		if(IsLeaf(pNode))
		{
			// leaf node
			str.append("(");
			for(uint8_t i=0; i<DimensionValue; ++i)
				str.append((boost::format("%s, ") % KeySerialization<KeyT>::Serialization(pNode->Vector[i])).str());
			str.erase(str.length() - 2);
			str.append(")");
		}
		else
		{
			// inner node
			str.append((boost::format("[split:%hhu, value:%s]") %
							(uint32_t)pNode->Head.SplitDimension %
							KeySerialization<KeyT>::Serialization(pNode->Vector[pNode->Head.SplitDimension])).str());
		}
		return str;
	}

	void PrintNode(KDNode<ValueT, DimensionValue, KeyT, IndexT>* node, std::vector<bool>::size_type layer, bool isRight, std::vector<bool>& flags)
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
			{
				printf("\033[%sm%snode: %s[p%u:c%u:l%u:r%u]\033[0m\n", 
							IsLeaf(node)?"33":"34",
							isRight?"r":"l", 
							NodeSerialization(node).c_str(), 
							node->Head.ParentIndex, 
							nodeIndex,
							node->Head.LeftIndex, 
							node->Head.RightIndex);
			}
			else
			{
				printf("\033[36m%snode: (null)\033[0m\n", isRight?"r":"l");
				return;
			}
		}
		else
		{
			printf("\\-");
			if(node)
			{
				printf("\033[%smroot: %s[p%u:c%u:l%u:r%u]\033[0m\n",
							IsLeaf(node)?"33":"34",
							NodeSerialization(node).c_str(),
							node->Head.ParentIndex,
							nodeIndex,
							node->Head.LeftIndex,
							node->Head.RightIndex);
			}
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
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* leftNode = m_NodeBlockTable[node->Head.LeftIndex];
		PrintNode(leftNode, layer+1, false, flags);

		KDNode<ValueT, DimensionValue, KeyT, IndexT>* rightNode = m_NodeBlockTable[node->Head.RightIndex];
		PrintNode(rightNode, layer+1, true, flags);
	}

	BlockTable<KDNode<ValueT, DimensionValue, KeyT, IndexT>, 
				KDTreeHead<IndexT>, IndexT> m_NodeBlockTable;
};

#endif // define __KDTREE_HPP__
