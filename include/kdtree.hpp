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

#ifdef KDTREE_USE_DRAW2DMAP
#include <Magick++.h>
#endif // define KDTREE_USE_DRAW2DMAP

#include "keyutility.hpp"
#include "blocktable.hpp"
#include "heap.hpp"
#include "distance.hpp"

#define MAX_LOOPCOUNT		1000

#define IMG_SIZE	860
#define ZOOM		IMG_SIZE / 100

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
	uint32_t Count;
} __attribute__((packed));

template<typename ValueT, uint8_t DimensionValue, 
			typename KeyT = uint32_t, template<typename, typename, uint8_t> class DistanceT = EuclideanDistance, 
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
												KDTreeHead<IndexT>, IndexT>::CreateBlockTable(2*size);
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
							KDTreeHead<IndexT>, IndexT>::GetBufferSize(2*size);
	}

	static inline KeyT Distance(VectorType& v1, VectorType& v2)
	{
		return DistanceT<KeyT, KDVector<KeyT, DimensionValue>, DimensionValue>::Distance(v1, v2);
	}

	void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	int Build(std::vector<DataType>& vData)
	{
		if(vData.size() == 0)
			return -1;

		std::sort(vData.begin(), vData.end(), DataCompare(DataCompare::COMPARE_VECTOR));
		typename std::vector<DataType>::iterator iter = std::unique(vData.begin(), vData.end(), DataCompare(DataCompare::COMPARE_EQUAL));
		vData.resize(std::distance(vData.begin(), iter));

		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		pHead->Count = vData.size();
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pRootNode = m_NodeBlockTable[pHead->RootIndex];
		if(pRootNode)
			return -1;

		return BuildTree(&pHead->RootIndex, NULL, vData.begin(), vData.end());
	}

	size_t Nearest(VectorType key, DataType* buffer, size_t size)
	{
		if(buffer == NULL || size == 0)
			return 0;

		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		if(!m_NodeBlockTable[pHead->RootIndex])
			return 0;

		MinimumHeap<KeyT, IndexT> miniheap = MinimumHeap<KeyT, IndexT>::CreateHeap(MAX_LOOPCOUNT);
		MaximumHeap<KeyT, DataType> nearestMaxHeap = MaximumHeap<KeyT, DataType>::CreateHeap(size);

		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode = FindToLeaf(key, pHead->RootIndex, miniheap);
		DataType* pData = nearestMaxHeap.Push(DistanceT<KeyT, KDVector<KeyT, DimensionValue>, DimensionValue>::Distance(pNode->Vector, key));
		pData->Vector = pNode->Vector;
		pData->Value = pNode->Value;

		int loop = 0;
		while(miniheap.Count() > 0 && loop < MAX_LOOPCOUNT)
		{
			KeyT hyperplaneDistance;
			IndexT miniOtherIndex = *miniheap.Minimum(&hyperplaneDistance);
			miniheap.Pop();

			KeyT maxDistance = 0;
			nearestMaxHeap.Maximum(&maxDistance);

			if(hyperplaneDistance < maxDistance || nearestMaxHeap.Count() < size)
			{
				pNode = FindToLeaf(key, miniOtherIndex, miniheap);
				KeyT distance = DistanceT<KeyT, KDVector<KeyT, DimensionValue>, DimensionValue>::Distance(pNode->Vector, key);
				if(distance < maxDistance || nearestMaxHeap.Count() < size)
				{
					if(nearestMaxHeap.Count() >= size)
						nearestMaxHeap.Pop();
					DataType* pData = nearestMaxHeap.Push(distance);
					pData->Vector = pNode->Vector;
					pData->Value = pNode->Value;
				}
			}
			++loop;
		}

		size_t foundCount = nearestMaxHeap.Count();
		for(int i=foundCount-1; i>=0; --i)
		{
			memcpy(&buffer[i], nearestMaxHeap.Maximum(), sizeof(DataType));
			nearestMaxHeap.Pop();
		}

		miniheap.Delete();
		nearestMaxHeap.Delete();
		return foundCount;
	}

	size_t Range(VectorType key, KeyT range, DataType* buffer, size_t size)
	{
		if(buffer == NULL || size == 0)
			return 0;

		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();
		if(!m_NodeBlockTable[pHead->RootIndex])
			return 0;

		MinimumHeap<KeyT, IndexT> miniheap = MinimumHeap<KeyT, IndexT>::CreateHeap(MAX_LOOPCOUNT);
		MaximumHeap<KeyT, DataType> nearestMaxHeap = MaximumHeap<KeyT, DataType>::CreateHeap(size);

		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode = FindToLeafWithRange(key, range, pHead->RootIndex, miniheap);
		KeyT distance = DistanceT<KeyT, KDVector<KeyT, DimensionValue>, DimensionValue>::Distance(pNode->Vector, key);
		if(distance <= range)
		{
			DataType* pData = nearestMaxHeap.Push(distance);
			pData->Vector = pNode->Vector;
			pData->Value = pNode->Value;
		}

		int loop = 0;
		while(miniheap.Count() > 0 && loop < MAX_LOOPCOUNT)
		{
			KeyT hyperplaneDistance;
			IndexT miniOtherIndex = *miniheap.Minimum(&hyperplaneDistance);
			miniheap.Pop();

			KeyT maxDistance = 0;
			nearestMaxHeap.Maximum(&maxDistance);

			if(hyperplaneDistance < maxDistance || nearestMaxHeap.Count() < size)
			{
				pNode = FindToLeafWithRange(key, range, miniOtherIndex, miniheap);
				distance = DistanceT<KeyT, KDVector<KeyT, DimensionValue>, DimensionValue>::Distance(pNode->Vector, key);
				if(distance <= range && (distance < maxDistance || nearestMaxHeap.Count() < size))
				{
					if(nearestMaxHeap.Count() >= size)
						nearestMaxHeap.Pop();
					DataType* pData = nearestMaxHeap.Push(distance);
					pData->Vector = pNode->Vector;
					pData->Value = pNode->Value;
				}
			}
			++loop;
		}

		size_t foundCount = nearestMaxHeap.Count();
		for(int i=foundCount-1; i>=0; --i)
		{
			memcpy(&buffer[i], nearestMaxHeap.Maximum(), sizeof(DataType));
			nearestMaxHeap.Pop();
		}

		miniheap.Delete();
		nearestMaxHeap.Delete();
		return foundCount;
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
		printf("Node Count: %u\n", pHead->Count);
		PrintNode(node, 0, false, flags);
	}

#ifdef KDTREE_USE_DRAW2DMAP

	void DrawNode(Magick::Image& img, KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode, double x, double y, double ex, double ey)
	{
		if(!pNode)
			return;

		if(IsLeaf(pNode))
		{
			img.strokeColor("red");
			img.strokeWidth(1);
			img.fillColor("red");

			double nodeX = pNode->Vector[0] * ZOOM;
			double nodeY = pNode->Vector[1] * ZOOM;
			Magick::DrawableCircle circle(nodeX, nodeY, nodeX-5, nodeY);
			img.draw(circle);

			img.strokeColor("black");
			Magick::DrawableText txt(nodeX-25, nodeY-12, (boost::format("(%u, %u)") % pNode->Vector[0] % pNode->Vector[1]).str());
			img.draw(txt);

			return;
		}
		else
		{
			img.strokeColor("black");
			img.strokeWidth(1);
			Magick::DrawableLine line(0, 0, 0, 0);

			double lx, ly, lex, ley;
			double rx, ry, rex, rey;
			if(pNode->Head.SplitDimension)
			{
				line.startX(x);
				line.startY(ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension]);
				line.endX(ex);
				line.endY(ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension]);

				lx = x;
				ly = y;
				lex = ex;
				ley = ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension];

				rx = x;
				ry = ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension];
				rex = ex;
				rey = ey;
			}
			else
			{
				line.startX(ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension]);
				line.startY(y);
				line.endX(ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension]);
				line.endY(ey);

				lx = x;
				ly = y;
				lex = ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension];
				ley = ey;

				rx = ZOOM * (double)pNode->Vector[pNode->Head.SplitDimension];
				ry = y;
				rex = ex;
				rey = ey;
			}
			img.draw(line);

			DrawNode(img, m_NodeBlockTable[pNode->Head.LeftIndex], lx, ly, lex, ley);
			DrawNode(img, m_NodeBlockTable[pNode->Head.RightIndex], rx, ry, rex, rey);
		}
	}

	void Draw2DMap(const char* szFile, double searchX, double searchY)
	{
		if(!szFile || DimensionValue != 2)
			return;

		KDTreeHead<IndexT>* pHead = m_NodeBlockTable.GetHead();

		Magick::Color backgroundColor(0xFFFF, 0xFFFF, 0xFFFF);
		Magick::Geometry size(IMG_SIZE, IMG_SIZE);
		Magick::Image img(size, backgroundColor);

		DrawNode(img, m_NodeBlockTable[pHead->RootIndex], 0, 0, IMG_SIZE, IMG_SIZE);

		img.strokeColor("green");
		img.strokeWidth(1);
		img.fillColor("green");

		double nodeX = searchX * ZOOM;
		double nodeY = searchY * ZOOM;
		Magick::DrawableCircle circle(nodeX, nodeY, nodeX-5, nodeY);
		img.draw(circle);

		img.strokeColor("black");
		Magick::DrawableText txt(nodeX-25, nodeY-12, (boost::format("(%u, %u)") % searchX % searchY).str());
		img.draw(txt);

		img.write(szFile);
	}

#endif // define KDTREE_USE_DRAW2DMAP

	KDTree()
	{
	}

protected:

	KDNode<ValueT, DimensionValue, KeyT, IndexT>* FindToLeafWithRange(VectorType key, KeyT range, IndexT nodeIndex, MinimumHeap<KeyT, IndexT>& min_pq)
	{
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode = NULL;
		while((pNode = m_NodeBlockTable[nodeIndex]) && !IsLeaf(pNode))
		{
			KeyT distance = (KeyT)fabs(pNode->Vector[pNode->Head.SplitDimension] - key[pNode->Head.SplitDimension]);
			if(key[pNode->Head.SplitDimension] < pNode->Vector[pNode->Head.SplitDimension])
			{
				nodeIndex = pNode->Head.LeftIndex;
				if(distance <= range)
				{
					IndexT* pValue = min_pq.Push(distance);
					if(pValue)
						*pValue = pNode->Head.RightIndex;
				}
			}
			else
			{
				nodeIndex = pNode->Head.RightIndex;
				if(distance <= range)
				{
					IndexT* pValue = min_pq.Push(distance);
					if(pValue)
						*pValue = pNode->Head.LeftIndex;
				}
			}
		}
		return pNode;
	}

	KDNode<ValueT, DimensionValue, KeyT, IndexT>* FindToLeaf(VectorType key, IndexT nodeIndex, MinimumHeap<KeyT, IndexT>& min_pq)
	{
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode = NULL;
		while((pNode = m_NodeBlockTable[nodeIndex]) && !IsLeaf(pNode))
		{
			KeyT distance = (KeyT)fabs(pNode->Vector[pNode->Head.SplitDimension] - key[pNode->Head.SplitDimension]);
			if(key[pNode->Head.SplitDimension] < pNode->Vector[pNode->Head.SplitDimension])
			{
				nodeIndex = pNode->Head.LeftIndex;
				IndexT* pValue = min_pq.Push(distance);
				if(pValue)
					*pValue = pNode->Head.RightIndex;
			}
			else
			{
				nodeIndex = pNode->Head.RightIndex;
				IndexT* pValue = min_pq.Push(distance);
				if(pValue)
					*pValue = pNode->Head.LeftIndex;
			}
		}
		return pNode;
	}

	int BuildTree(IndexT* pNodeIndex, KDNode<ValueT, DimensionValue, KeyT, IndexT>* pParentNode,
					typename std::vector<DataType>::iterator iterBegin, 
					typename std::vector<DataType>::iterator iterEnd)
	{
		*pNodeIndex = m_NodeBlockTable.AllocateBlock();
		KDNode<ValueT, DimensionValue, KeyT, IndexT>* pNode = m_NodeBlockTable[*pNodeIndex];
		if(!pNode)
			return -1;
		
		memset(pNode, 0, sizeof(KDNode<ValueT, DimensionValue, KeyT, IndexT>));
		pNode->Head.ParentIndex = m_NodeBlockTable.GetBlockID(pParentNode);
		if(std::distance(iterBegin, iterEnd) == 1)
		{
			// leaf node
			pNode->Vector = iterBegin->Vector;
			pNode->Value = iterBegin->Value;
		}
		else
		{
			// inner node
			pNode->Head.SplitDimension = GetMaxVarianceDimension(iterBegin, iterEnd);
			pNode->Vector = GetMedianVector(pNode->Head.SplitDimension, iterBegin, iterEnd);

			typename std::vector<DataType>::iterator iterMedian = iterBegin + std::distance(iterBegin, iterEnd) / 2;
			if(0 != BuildTree(&pNode->Head.LeftIndex, pNode, iterBegin, iterMedian) ||
				0 != BuildTree(&pNode->Head.RightIndex, pNode, iterMedian, iterEnd))
				return -1;
		}
		return 0;
	}

	uint8_t GetMaxVarianceDimension(typename std::vector<DataType>::iterator iterBegin,
									typename std::vector<DataType>::iterator iterEnd)
	{
		uint8_t maxVarianceDimension = 0;

		KeyT maxVariance = 0;
		for(uint8_t i=0; i<DimensionValue; ++i)
		{
			KeyT mean = 0;
			KeyT var = 0;
			for(typename std::vector<DataType>::iterator iter = iterBegin;
				iter != iterEnd;
				++iter)
			{
				mean += iter->Vector[i];
			}
			mean /= std::distance(iterBegin, iterEnd);

			for(typename std::vector<DataType>::iterator iter = iterBegin;
				iter != iterEnd;
				++iter)
			{
				KeyT sub = (KeyT)fabs(iter->Vector[i] - mean);
				var += sub * sub;
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

	VectorType GetMedianVector(uint8_t dim, 
								typename std::vector<DataType>::iterator iterBegin,
								typename std::vector<DataType>::iterator iterEnd)
	{
		typename std::vector<DataType>::size_type median = std::distance(iterBegin, iterEnd) / 2;

		DataCompare compare(DataCompare::COMPARE_DIMENSION);
		compare.SetCompareDimension(dim);
		std::nth_element(iterBegin, iterBegin + median, iterEnd, compare);

		return (iterBegin + median)->Vector;
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
