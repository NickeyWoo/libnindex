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
#include "hashtable.hpp"

struct Key
{
	uint32_t	dwUserId;
	uint32_t	dwOrderId;
};

struct Value
{
	uint32_t	dwUserId;
	char		sName[16];
	char		cStatus;
	double		dAmount;
} __attribute__((packed));

struct A
{
	bool isred;
} __attribute__((packed));

int main(int argc, char* argv[])
{
	Seed seed(10, 10);
	FileStorage fs;
	if(FileStorage::OpenStorage(&fs, "./database.data", HashTable<Key&, Value>::GetBufferSize(seed)) < 0)
	{
		printf("error: open data file fail.\n");
		return -1;
	}

	HashTable<Key&, Value> ht = HashTable<Key&, Value>::LoadHashTable(fs, seed);

	uint32_t i=0;
	for(; i<5000; ++i)
	{
		Key key;
		memset(&key, 0, sizeof(Key));
		key.dwUserId = random();
		key.dwOrderId = random();

		Value* pItem = ht.Hash(key, true);
		if(pItem == NULL)
			break;

		pItem->dwUserId = random();
		pItem->cStatus = 0xFF;
		pItem->dAmount = 100.55;
		sprintf(pItem->sName, "%lu", random());
	}

	printf("insert count: %d\n", i);

	// dump hashtable buffer
	ht.Dump();

	fs.Release();
	return 0;
}




