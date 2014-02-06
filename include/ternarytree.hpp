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
#include "blocktable.hpp"

template<typename IndexT>
struct TernaryNodeHead
{
	IndexT ParentIndex;
	IndexT LeftIndex;
	IndexT CenterIndex;
	IndexT RightIndex;
} __attribute__((packed));

template<typename KeyT, typename IndexT>
struct TernaryNode
{
	TernaryNodeHead<IndexT> Head;
	KeyT Key;
} __attribute__((packed));

template<typename IndexT>
struct TernaryTreeHead
{
	IndexT RootIndex;
} __attribute__((packed));

template<typename KeyT, typename IndexT = uint32_t>
class TernaryTree
{
public:
	static TernaryTree<KeyT, IndexT> CreateTernaryTree(size_t size)
	{
		TernaryTree<KeyT, IndexT> tt;
		tt.m_NodeBlockTable = BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::CreateBlockTable(size);
		return tt;
	}

	static TernaryTree<KeyT, IndexT> LoadTernaryTree(char* buffer, size_t size)
	{
		TernaryTree<KeyT, IndexT> tt;
		tt.m_NodeBlockTable = BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::LoadBlockTable(buffer, size);
		return tt;
	}

	template<typename StorageT>
	static inline TernaryTree<KeyT, IndexT> LoadTernaryTree(StorageT storage)
	{
		return LoadTernaryTree(storage.GetBuffer(), storage.GetSize());
	}

	static inline size_t GetBufferSize(IndexT stringCount, IndexT avgStringLength)
	{
		return BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::GetBufferSize(stringCount * avgStringLength);
	}

	static inline size_t GetBufferSize(IndexT size)
	{
		return BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT>::GetBufferSize(size);
	}

protected:

	BlockTable<TernaryNode<KeyT, IndexT>, TernaryTreeHead<IndexT>, IndexT> m_NodeBlockTable;

};


#endif // define __TERNARYTREE_HPP__
