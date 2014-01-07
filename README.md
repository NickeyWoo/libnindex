# N-Index Library
N-Index is common data index and storage library.

## Index Struct ##
* **HashTable<KeyT, ValueT>**
* **Bitmap<KeyT>**
* **BloomFilter<KeyT>**
* **BlockTable<ValueT>**
* **RBTree<KeyT, ValueT>**

## Example ##
**HashTable:** [hashtable_main.cpp][2]
```c++
	struct MyKey {
		uint64_t ddwUserId;
		uint32_t dwSubId;
	};
	struct MyValue {
		char sName[16];
	} __attribute__((packed));

	// load hash table;
	HashTable<MyKey&, MyValue> ht = HashTable<MyKey&, MyValue>::LoadHashTable(...);

	// initialize key
	MyKey key;
	memset(&key, 0, sizeof(MyKey));
	key.ddwUserId = 2657910038;
	key.dwSubId = 2000;

	// find value
	MyValue* pItem = ht.Hash(key);
	if(pItem != NULL)
		printf("name: %s\n", pItem->sName);
```

**Bitmap:** [bitmap_main.cpp][3]
```c++
	Bitmap<uint64_t> bm = Bitmap<uint64_t>::CreateBitmap(4096);

	uint64_t ddwTweetId = 312590019750805;

	// test for the presence
	if(bm.Contains(ddwTweetId))
		return;

	// set the bit
	bm.Set(ddwTweetId);

	bm.Delete();
```

**BloomFilter** [bloomfilter_main.cpp][4]
```c++
	BloomFilter<std::string> bf = BloomFilter<std::string>::LoadBloomFilter(...);

	std::string strCrawlUrl = Scheduler.Next();

	// determine whether the url being crawled
	if(!bf.Contains(strCrawlUrl))
	{
		// crawl the url
		.....

		bf.Add(strCrawlUrl);
	}

```

**BlockTable** [blocktable_main.cpp][5]
```c++
	struct Tree {
		BlockTable<TreeNode, void>::BlockIndexType RootIndex;
	};
	struct TreeNode {
		uint64_t Key;
		uint32_t Value;

		BlockTable<TreeNode, void>::BlockIndexType ParentIndex;
		BlockTable<TreeNode, void>::BlockIndexType LeftIndex;
		BlockTable<TreeNode, void>::BlockIndexType RightIndex;
	};

	// load blocktable
	BlockTable<TreeNode, Tree> bt = BlockTable<TreeNode, Tree>::LoadBlockTable(...);

	BlockTable<TreeNode, void>::BlockIndexType newNodeIndex = bt.AllocateBlock();
	TreeNode* pNode = bt[newNodeIndex];

	Tree* pTree = bt.GetHead();
	bt.ReleaseBlock(pTree->RootIndex);
```

**RBTree** [rbtree_main.cpp][6]
```c++
	struct Key {
		uint32_t Uin;
		uint32_t Timestamp;
	} __attribute__((packed));
	
	template<>
	struct KeyCompare<Key>
	{
		static int Compare(Key key1, Key key2)
		{
			if(key1.Uin > key2.Uin)
				return 1;
			else if(key1.Uin < key2.Uin)
				return -1;
			else
				if(key2.Timestamp > key1.Timestamp)
					return 1;
				else if(key2.Timestamp < key1.Timestamp)
					return -1;
				else
					return 0;
		}
	};
				
	RBTree<Key, uint32_t> rbtree = RBTree<Key, uint32_t>::CreateRBTree(INSERT_NUM);

	// like SQL:
	// SELECT * FROM t WHERE Uin=1000 ORDER BY Timestamp DESC;

	Key vkeyBegin = {1000, 0xffffffff};
	Key vkeyEnd = {1001, 0};
	RBTree<Key, uint32_t>::RBTreeIterator iter = rbtree.Iterator(vkeyBegin);
	RBTree<Key, uint32_t>::RBTreeIterator iterEnd = rbtree.Iterator(vkeyEnd);
	
	// like SQL:
	// SELECT COUNT(*) FROM t WHERE Uin=1000;
	printf("Count: %u\n", rbtree.Count(iter, iterEnd));

	uint32_t* pValue = NULL;
	Key key;
	while(iter != iterEnd && (pValue = rbtree.Next(&iter, &key)))
	{
		printf("Key:(uin:%u, timestamp:%u), Value:%u\n", key.Uin, key.Timestamp, *pValue);
	}

	rbtree.Delete();
```

[More examples...][1]

  [1]: https://github.com/NickeyWoo/nindex/tree/master/example
  [2]: https://github.com/NickeyWoo/nindex/blob/master/example/hashtable_main.cpp
  [3]: https://github.com/NickeyWoo/nindex/blob/master/example/bitmap_main.cpp
  [4]: https://github.com/NickeyWoo/nindex/blob/master/example/bloomfilter_main.cpp
  [5]: https://github.com/NickeyWoo/nindex/blob/master/example/blocktable_main.cpp
  [6]: https://github.com/NickeyWoo/nindex/blob/master/example/rbtree_main.cpp

