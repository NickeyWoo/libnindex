#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <utility>
#include <vector>
#include <string>
#include <boost/format.hpp>
#include "utility.hpp"
#include "storage.hpp"
#include "rbtree.hpp"

struct Key
{
	uint32_t Uin;
	uint32_t Timestamp;
	uint32_t Count;
} __attribute__((packed));

struct Value
{
	uint32_t Num;
	char sName[16];
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
		{
			if(key1.Timestamp > key2.Timestamp)
				return 1;
			else if(key1.Timestamp < key2.Timestamp)
				return -1;
			else
			{
				if(key2.Count > key1.Count)
					return 1;
				else if(key2.Count < key1.Count)
					return -1;
				else
					return 0;
			}
		}
	}
};

template<>
struct KeySerialization<Key>
{
	static std::string Serialization(Key key)
	{
		uint32_t uin = key.Uin;
		uint32_t timestamp = key.Timestamp;
		uint32_t count = key.Count;
		std::string str = (boost::format("uin:%u timestamp:%03u count:%02u") % uin % timestamp % count).str();
		return str;
	}
};

#define INSERT_NUM 30
#define DELETE_NUM 15

int main(int argc, char* argv[])
{
/*
	FileStorage storage;
	FileStorage::OpenStorage(&storage, "rbtree.data", RBTree<Key, uint32_t>::GetBufferSize(INSERT_NUM));
	RBTree<Key, uint32_t> rbtree = RBTree<Key, uint32_t>::LoadRBTree(storage);
*/
	RBTree<Key, uint32_t> rbtree = RBTree<Key, uint32_t>::CreateRBTree(INSERT_NUM);

	Key delKeyBuffer[DELETE_NUM];
	for(int i=0; i<INSERT_NUM; ++i)
	{
		Key key;
		memset(&key, 0, sizeof(Key));
		key.Uin = random() % 3;
		key.Timestamp = random() % 1000;
		key.Count = random() % 100;

		if(i % 2 == 0)
			memcpy(&delKeyBuffer[i/2], &key, sizeof(Key));

		uint32_t* pValue = rbtree.Hash(key, true);
		*pValue = i;
	}

	printf("\nBefore:\n");
	rbtree.DumpTree();

	for(int i=0; i<DELETE_NUM; ++i)
	{
		rbtree.Clear(delKeyBuffer[i]);
	}

	printf("\nAfter:\n");
	rbtree.DumpTree();

	printf("\n//////////////////////////////////////////////////////////////////\nAll Data:\n");

	Key key;
	uint32_t* pValue = NULL;
	RBTree<Key, uint32_t>::RBTreeIterator iter = rbtree.Iterator();
	while((pValue = rbtree.Next(&iter, &key)))
	{
		printf("Next:%02u Key:(%s)\n", iter.Index, KeySerialization<Key>::Serialization(key).c_str());
	}

	printf("\n//////////////////////////////////////////////////////////////////\nSELECT * FROM t WHERE Uin=1 AND Timestamp>=700 ORDER BY Count DESC;\n");

	Key vkeyBegin = {1, 700, 0xffffffff};
	iter = rbtree.Iterator(vkeyBegin);

	Key vkeyEnd = {2, 0, 0};
	RBTree<Key, uint32_t>::RBTreeIterator iterEnd = rbtree.Iterator(vkeyEnd);

	while((pValue = rbtree.Next(&iter, &key)))
	{
		printf("Next:%02u End:%02u Key:(%s)\n", iter.Index, iterEnd.Index, KeySerialization<Key>::Serialization(key).c_str());

		if(iter == iterEnd)
			break;
	}

	pValue = rbtree.Next(&iterEnd, &key);
	if(pValue)
		printf("End Next:%02u Key:(%s)\n", iter.Index, KeySerialization<Key>::Serialization(key).c_str());

	rbtree.Delete();
	// storage.Release();
	return 0;
}




