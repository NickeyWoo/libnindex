# N-Index Library
N-Index is common data index and storage library.

## Index Struct ##
* **HashTable**
* **TimerHashTable**
* **Bitmap**
* **BloomFilter**
* **BlockTable**
* **MultiBlockTable**
* **RBTree**
* **Heap**
* **KDTree**
* **TernarySearchTree**

## Example ##
**HashTable / TimerHashTable** [hashtable_main.cpp][2]
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

**Bitmap** [bitmap_main.cpp][3]
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

**BlockTable / MultiBlockTable** [blocktable_main.cpp][5]
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

**Heap** [heap_main.cpp][7]
```c++
	struct Value {
		uint64_t Data;
	];

	MinimumHeap<uint32_t, Value> min = MinimumHeap<uint32_t, Value>::CreateHeap(10);
	MaximumHeap<uint32_t, Value> max = MaximumHeap<uint32_t, Value>::CreateHeap(10);

	for(uint32_t i=0; i<10; ++i)
	{
		uint32_t key = random() % 9;
		min.Push(key)->Data = random() % 79;
		max.Push(key)->Data = random() % 99;
	}

	Value* pMinValue = min.Minimum();
	Value* pMaxValue = max.Maximum();

	min.Delete();
	max.Delete();
```

**KDTree** [kdtree_main.cpp][8]
```c++
	struct Store {
		uint64_t StoreId;
	];

	Store* pBuffer = (Store*)malloc(sizeof(Store)*10);

	gps = {x, y};
	uint32_t range = 100;
	kdtree.Range(gps, range, pBuffer, 10);

	kdtree.Delete();
```

**TernarySearchTree** [ternarytree_main.cpp][9]
```c++
	struct Value {
		uint32_t TweetID;
	} __attribute__((packed));

	struct dict {
		const char* str;
	} dicts[] = { {"alpha"}, {"abs"}, {"a"}, {"bay"}, {"baby"}, {"大家好"}, {"大学"}, {"大小"} };

	TernaryTree<Value> tt = TernaryTree<Value>::CreateTernaryTree(sizeof(dicts)/sizeof(dict), 10);
	for(size_t i=0; i<sizeof(dicts)/sizeof(dict); ++i) {
		Value* pValue = tt.Hash(dicts[i].str, true);
		if(!pValue)
			break;
		pValue->TweetID = i;
	}

	const char* pPrefixStr = "大";
	TernaryTree<Value>::TernaryTreeIterator iter = tt.PrefixSearch(pPrefixStr);
	char buffer[10];
	memset(buffer, 0, 10);
	size_t size = 0;
	Value* pValue = NULL;
	while((pValue = tt.Next(&iter, buffer, 10, &size)) != NULL)
	{
		printf("  %s, TweetID:%u\n", buffer, pValue->TweetID);
		memset(buffer, 0, size);
	}
```

[More examples...][1]

[libnindex doc][10]

[libnindex ppt][11]


  [1]: https://github.com/NickeyWoo/libnindex/tree/master/example
  [2]: https://github.com/NickeyWoo/libnindex/tree/master/example/hashtable_main.cpp
  [3]: https://github.com/NickeyWoo/libnindex/tree/master/example/bitmap_main.cpp
  [4]: https://github.com/NickeyWoo/libnindex/tree/master/example/bloomfilter_main.cpp
  [5]: https://github.com/NickeyWoo/libnindex/tree/master/example/blocktable_main.cpp
  [6]: https://github.com/NickeyWoo/libnindex/tree/master/example/rbtree_main.cpp
  [7]: https://github.com/NickeyWoo/libnindex/tree/master/example/heap_main.cpp
  [8]: https://github.com/NickeyWoo/libnindex/tree/master/example/kdtree_main.cpp
  [9]: https://github.com/NickeyWoo/libnindex/tree/master/example/ternarytree_main.cpp
  [10]: https://github.com/NickeyWoo/libnindex/blob/master/docs/libnindex%E4%B9%8B%E5%90%8E%E5%8F%B0%E5%B8%B8%E7%94%A8%E7%B4%A2%E5%BC%95%E6%8A%80%E6%9C%AF%E6%B5%85%E6%9E%90.docx?raw=true
  [11]: https://github.com/NickeyWoo/libnindex/blob/master/docs/nindex.pptx?raw=true



