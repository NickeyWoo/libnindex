/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2014.06.11
 *
*--*/
#ifndef __BTREE_HPP__
#define __BTREE_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include "utility.hpp"
#include "blockfile.hpp"

template<typename KeyT, size_t BlockSize = DEFAULT_BLOCKSIZE>
struct AutoDegree
{
	enum {
		Degree = (BlockSize - sizeof(uint8_t) - sizeof(uint16_t) - sizeof(off_t)) / 
					(sizeof(KeyT) + sizeof(off_t))
	};
};

template<typename KeyT, size_t Degree>
struct BTreeIndexNode
{
	uint8_t 			IsLeaf;

	uint16_t 			KeyNum;
	KeyT 				Keys[Degree];
	off_t 				Offsets[Degree + 1];
};

template<typename KeyT, typename ValueT, size_t Degree>
struct BTreeLeafNode
{
	uint8_t 			IsLeaf;
	off_t 				NextNode;

	uint16_t 			KeyNum;
	KeyT 				Keys[Degree];
	ValueT 				Values[Degree];
};

template<typename KeyT, typename ValueT, typename FileT = BlockFile<void> >
class BTree
{
public:
	typedef BTree<KeyT, ValueT, FileT> BTreeType;

	static BTreeType CreateBTree(FileT file)
	{
		BTreeType btree;
		btree.m_BlockFile = file;
		return btree;
	}

	static BTreeType LoadBTree(FileT file)
	{
		BTreeType btree;
		btree.m_BlockFile = file;
		return btree;
	}

private:
	FileT m_BlockFile;
};

#endif // define __BTREE_HPP__
