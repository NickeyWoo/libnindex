# n-index
nindex is common data storage and index library.

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
	// see example
```

**RBTree** [rbtree_main.cpp][6]
```c++
	// see example
```

[More examples...][1]

  [1]: https://github.com/NickeyWoo/nindex/tree/master/example
  [2]: https://github.com/NickeyWoo/nindex/blob/master/example/hashtable_main.cpp
  [3]: https://github.com/NickeyWoo/nindex/blob/master/example/bitmap_main.cpp
  [4]: https://github.com/NickeyWoo/nindex/blob/master/example/bloomfilter_main.cpp
  [5]: https://github.com/NickeyWoo/nindex/blob/master/example/blocktable_main.cpp
  [6]: https://github.com/NickeyWoo/nindex/blob/master/example/rbtree_main.cpp

