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
	uint64_t TweetID;
} __attribute__((packed));

#define INSERT_NUM	10

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
	TernaryTree<Value> tt = TernaryTree<Value>::CreateTernaryTree(INSERT_NUM, 10);

	printf("dicts:\n");
	for(size_t i=0; i<INSERT_NUM; ++i)
	{
		char buffer[9];
		memset(buffer, 0, 9);

		size_t lenString = random() % 8;
		for(size_t j=0; j<lenString; ++j)
		{
			buffer[j] = 0x21 + random() % 0x5D;
		}
		printf("[%lu]: %s\n", i, buffer);
		
		Value* pValue = tt.Hash(buffer, true);
		if(!pValue)
			break;

		pValue->TweetID = i;

		tt.Dump();
		exit(0);
	}

	//fs.Flush();
	//fs.Release();
	return 0;
}

