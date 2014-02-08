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
#include "blocktable.hpp"

struct Value
{
	uint8_t Number;
} __attribute__((packed));

struct Value1
{
	uint8_t Value1;
} __attribute__((packed));

struct Value2
{
	uint16_t Value2;
} __attribute__((packed));

struct Value3
{
	uint32_t Value3;
} __attribute__((packed));

int main(int argc, char* argv[])
{
	FileStorage fs;
	FileStorage::OpenStorage(&fs, "./blocktable.data", BlockTable<Value, void, uint16_t>::GetBufferSize(20));

	BlockTable<Value, void, uint16_t> bt = BlockTable<Value, void, uint16_t>::LoadBlockTable(fs);

	// 填充一部分blocktable空间
	for(uint16_t i=0; i<15; ++i)
	{
		uint16_t id = bt.AllocateBlock();
		Value* pItem = bt[id];
		if(!pItem)
			break;

		printf("alloc block: %d\n", id);
		bt[id]->Number = i;
	}

	// 随机释放一部分空间
	for(uint16_t i=0; i<10; ++i)
	{
		uint16_t id = random() % 19;
		Value* pItem = bt[id];
		if(pItem)
		{
			printf("release block: %d\n", id);
			bt.ReleaseBlock(id);
		}
	}

	// 重新填满整个blocktable空间
	for(uint16_t i=0; i<20; ++i)
	{
		uint16_t id = bt.AllocateBlock();
		Value* pItem = bt[id];
		if(!pItem)
			break;

		printf("alloc block: %d\n", id);
		bt[id]->Number = i;
	}

	bt.Dump();
	
	printf("\n\n/////////////////////////////////////////////////////////\n");

	std::vector<uint32_t> vSize;
	vSize.push_back(20);
	vSize.push_back(10);
	vSize.push_back(5);

	FileStorage fs2;
	FileStorage::OpenStorage(&fs2, "./multiblocktable.data", MultiBlockTable<TYPELIST_3(Value1, Value2, Value3)>::GetBufferSize(vSize));

	MultiBlockTable<TYPELIST_3(Value1, Value2, Value3)> mbt = MultiBlockTable<TYPELIST_3(Value1, Value2, Value3)>::LoadMultiBlockTable(fs2, vSize);

	for(uint8_t i=0; i<15; ++i)
	{
		Value1* pVal1 = NULL;
		mbt.AllocateBlock(&pVal1);

		if(!pVal1)
			break;
	}

	for(uint8_t i=0; i<15; ++i)
	{
		Value1* pVal1 = NULL;
		mbt.AllocateBlock(&pVal1);

		if(!pVal1)
			break;
	}

	mbt.Dump();

	return 0;
}




