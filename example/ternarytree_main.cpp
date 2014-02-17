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
#include "ternarytree.hpp"

struct Value
{
	uint32_t TweetID;
} __attribute__((packed));

template<>
struct KeySerialization<Value>
{
	static std::string Serialization(Value val)
	{
		uint32_t tid = val.TweetID;
		return (boost::format("TweetID:%u") % tid).str();
	}
};

struct dict
{
	const char* str;
};

dict dicts[] = {
	{"alpha"},
	{"abs"},
	{"a"},
	{"bay"},
	{"baby"},
	{"大家好"},
	{"大学"},
	{"大小"}
};

int main(int argc, char* argv[])
{
/*
	FileStorage fs;
	if(FileStorage::OpenStorage(&fs, "TernaryTree.index", TernaryTree<char>::GetBufferSize(100, 10)) < 0)
	{
		printf("error: open file fail.\n");
		return -1;
	}
	TernaryTree<Value> tt = TernaryTree<Value>::LoadTernaryTree(fs, 100, 10);
*/
	TernaryTree<Value> tt = TernaryTree<Value>::CreateTernaryTree(sizeof(dicts)/sizeof(dict), 10);

	printf("dicts:\n");
	for(size_t i=0; i<sizeof(dicts)/sizeof(dict); ++i)
	{
		Value* pValue = tt.Hash(dicts[i].str, true);
		if(!pValue)
			break;

		printf("[%lu] %s\n", i, dicts[i].str);
		pValue->TweetID = i;
	}

	tt.DumpTree();

	printf("\n");

	const char* pPrefixStr = "大";
	printf("prefix search: \"%s\"\n", pPrefixStr);

	char buffer[10];
	memset(buffer, 0, 10);

	TernaryTree<Value>::TernaryTreeIterator iter = tt.PrefixSearch(pPrefixStr);

	size_t size = 0;
	Value* pValue = NULL;
	while((pValue = tt.Next(&iter, buffer, 10, &size)) != NULL)
	{
		printf("  %s, TweetID:%u\n", buffer, pValue->TweetID);

		memset(buffer, 0, size);
	}

	//fs.Flush();
	//fs.Release();
	return 0;
}




