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
#include "keyutility.hpp"
#include "blocktable.hpp"
#include "distance.hpp"

template<typename KeyT, uint8_t Dimension>
struct NodeVector
{
	KeyT Key[Dimension];

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

template<typename ValueT, uint8_t Dimension, typename KeyT, typename IndexT>
struct KDNode
{
	KDNodeHead<IndexT> Head;

	NodeVector<KeyT, Dimension> Vector;
	ValueT Value;
} __attribute__((packed));

template<typename IndexT>
struct KDTreeHead
{
	IndexT RootIndex;
} __attribute__((packed));

template<typename ValueT, uint8_t Dimension, 
			typename KeyT = uint32_t, template<typename, uint8_t> class DistanceT = EuclideanDistance, 
			typename IndexT = uint32_t>
class KDTree
{
public:
	typedef NodeVector<KeyT, Dimension> KeyType;

	static KDTree<ValueT, Dimension, KeyT, DistanceT, IndexT> CreateKDTree(IndexT size)
	{
		KDTree<ValueT, Dimension, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, Dimension, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::CreateBlockTable(size);
		return kdtree;
	}

	static KDTree<ValueT, Dimension, KeyT, DistanceT, IndexT> LoadKDTree(char* buffer, size_t size)
	{
		KDTree<ValueT, Dimension, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, Dimension, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::LoadBlockTable(buffer, size);
		return kdtree;
	}

	template<typename StorageT>
	static KDTree<ValueT, Dimension, KeyT, DistanceT, IndexT> LoadKDTree(StorageT storage)
	{
		KDTree<ValueT, Dimension, KeyT, DistanceT, IndexT> kdtree;
		kdtree.m_NodeBlockTable = BlockTable<KDNode<ValueT, Dimension, KeyT, IndexT>, 
												KDTreeHead<IndexT>, IndexT>::LoadBlockTable(storage);
		return kdtree;
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return BlockTable<KDNode<ValueT, Dimension, KeyT, IndexT>, 
							KDTreeHead<IndexT>, IndexT>::GetBufferSize(size);
	}

	void Delete()
	{
		m_NodeBlockTable.Delete();
	}

	void Clear(KeyType pKey)
	{
	}

	int Insert(KeyType* pKeyBuffer, ValueT* pValueBuffer, size_t size)
	{
		return 0;
	}

	ValueT* Hash(KeyType pKey, bool isNew = false)
	{
		return NULL;
	}

	ValueT* Nearest(KeyType key)
	{
		return NULL;
	}

	ValueT* Nearest(KeyType key, size_t k)
	{
		return NULL;
	}

	ValueT* Range(KeyType key, KeyT range, ValueT* buffer, size_t size)
	{
		return NULL;
	}

	inline void Dump()
	{
		m_NodeBlockTable.Dump();
	}

	KDTree()
	{
	}

protected:

	BlockTable<KDNode<ValueT, Dimension, KeyT, IndexT>, 
				KDTreeHead<IndexT>, IndexT> m_NodeBlockTable;
};

#endif // define __KDTREE_HPP__
