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

	void Clear(VectorType Key)
	{
	}

	int OptimumImport(ImportDataType* pBuffer, size_t size)
	{
		if(!pBuffer || size == 0)
			return -1;

		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		IndexT CurIdx = pHead->RootIndex;
		IndexT ParentIdx = pHead->RootIndex;
		IndexT* pEmptyIdx = &pHead->RootIndex;
		KDNode<ValueT, Dimensions, KeyT, IndexT>* pNode = m_NodeBlockTable[pHead->RootIndex];

		// import data
		IndexT newIndex;
		ImportNodes(&newIndex, ParentIdx, pBuffer, size);
		KDNode<ValueT, Dimensions, KeyT, IndexT>* pNewNode = m_NodeBlockTable[newIndex];
		if(!pNewNode)
			return -1;

		// find insert postion
		while(pNode != NULL)
		{
			uint8_t vectorIndex = pNode->Head.SplitDimensions;
			ParentIdx = CurIdx;

			if(pNode->Vector == pNewNode->Vector)
				return -1;
			else if(pNode->Vector[vectorIndex] > pNewNode->Vector[vectorIndex])
			{
				CurIdx = pNode->Head.LeftIndex;
				pEmptyIdx = &pNode->Head.LeftIndex;
			}
			else
			{
				CurIdx = pNode->Head.RightIndex;
				pEmptyIdx = &pNode->Head.RightIndex;
			}
			pNode = m_NodeBlockTable[CurIdx];
		}

		*pEmptyIdx = newIndex;
		pNewNode->Head.ParentIndex = ParentIdx;
		return 0;
	}

	ValueT* Hash(VectorType key, bool isNew = false)
	{
		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		IndexT CurIdx = pHead->RootIndex;
		IndexT ParentIdx = pHead->RootIndex;

		IndexT* pEmptyIdx = &pHead->RootIndex;
		VectorType* pParentVector = NULL;
		KDNode<ValueT, Dimensions, KeyT, IndexT>* node = m_NodeBlockTable[pHead->RootIndex];
		while(node != NULL)
		{
			uint8_t vectorIndex = node->Head.SplitDimensions;
			if(node->Vector == key)
				return &node->Value;
			else if(key[vectorIndex] < node->Vector[vectorIndex])
			{
				ParentIdx = CurIdx;

				CurIdx = node->Head.LeftIndex;
				pParentVector = &node->Vector;
				pEmptyIdx = &node->Head.LeftIndex;

				node = m_NodeBlockTable[CurIdx];
			}
			else
			{
				ParentIdx = CurIdx;

				CurIdx = node->Head.RightIndex;
				pParentVector = &node->Vector;
				pEmptyIdx = &node->Head.RightIndex;

				node = m_NodeBlockTable[CurIdx];
			}
		}

		if(node == NULL && isNew)
		{
			*pEmptyIdx = m_NodeBlockTable.AllocateBlock();
			node = m_NodeBlockTable[*pEmptyIdx];
			if(!node)
				return NULL;

			memset(node, 0, sizeof(KDNode<ValueT, Dimensions, KeyT, IndexT>));
			node->Head.ParentIndex = ParentIdx;
			if(pParentVector)
				node->Head.SplitDimensions = GetOptimumDimensions(&key, 1, pParentVector);
			else
				node->Head.SplitDimensions = 0;
			memcpy(&node->Vector, &key, sizeof(VectorType));

			return &node->Value;
		}
		return NULL;
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

	void ImportNodes(IndexT* pIndex, IndexT ParentIdx, ImportDataType* pBuffer, size_t size)
	{
		if(!pIndex || !pBuffer || size == 0)
			return;

		*pIndex = m_NodeBlockTable.AllocateBlock();
		KDNode<ValueT, Dimensions, KeyT, IndexT>* pNode = m_NodeBlockTable[*pIndex];
		if(!pNode)
			return;

		memset(pNode, 0, sizeof(KDNode<ValueT, Dimensions, KeyT, IndexT>));

		VectorType meanVector;
		GetMeanVector(pBuffer, size, &meanVector);

		pNode->Head.ParentIndex = ParentIdx;
		pNode->Head.SplitDimensions = GetOptimumDimensions(pBuffer, size, &meanVector);

		ImportDataType* pMedianData = GetMedianData(pBuffer, size, pNode->Head.SplitDimensions);
		memcpy(&pNode->Vector, &pMedianData->Vector, sizeof(VectorType));
		memcpy(&pNode->Value, &pMedianData->Value, sizeof(ValueT));

		size_t leftSize = size / 2;
		ImportNodes(&pNode->Head.LeftIndex, *pIndex, pBuffer, leftSize);
		ImportNodes(&pNode->Head.RightIndex, *pIndex, &pBuffer[leftSize + 1], size - leftSize - 1);
	}

	VectorType* GetMeanVector(ImportDataType* pKey, size_t size, VectorType* pMeanVector)
	{
		if(!pMeanVector)
			return NULL;

		for(uint8_t i=0; i<Dimensions; ++i)
		{
			KeyT mean = 0;
			for(size_t j=0; j<size; ++j)
				mean += pKey[j].Vector[i];
			mean /= size;

			pMeanVector->Key[i] = mean;
		}
		return pMeanVector;
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

	template<typename DataT, typename S = void>
	struct VectorValue
	{
		static inline KeyT& GetValue(DataT* pData, uint8_t dim);
	};

	template<typename S>
	struct VectorValue<ImportDataType, S>
	{
		static inline KeyT& GetValue(ImportDataType* pData, uint8_t dim)
		{
			return pData->Vector[dim];
		}
	};

	template<typename S>
	struct VectorValue<VectorType, S>
	{
		static inline KeyT& GetValue(VectorType* pVector, uint8_t dim)
		{
			return pVector->Key[dim];
		}
	};

	template<typename DataT>
	uint8_t GetOptimumDimensions(DataT* pKey, size_t size, VectorType* pMeanVector)
	{
		KeyT maxVariance = 0;
		uint8_t optimumDimensions = 0;
		for(uint8_t i=0; i<Dimensions; ++i)
		{
			KeyT variance = 0;
			for(size_t j=0; j<size; ++j)
				variance += pow((VectorValue<DataT>::GetValue(&pKey[j], i) - pMeanVector->Key[i]), 2);
			variance /= size;

			if(maxVariance < variance)
			{
				maxVariance = variance;
				optimumDimensions = i;
			}
		}
		return optimumDimensions;
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
