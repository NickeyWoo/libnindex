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
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include <boost/format.hpp>
#include "utility.hpp"
#include "storage.hpp"
#include "kdtree.hpp"

struct Value {
	uint32_t Uin;
};

#define TIME_COST(t1, t2)			\
			(double)((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec)) / 1000000

#define INSERT_NUM 50

int main(int argc, char* argv[])
{
	KDTree<Value, 2> kdtree = KDTree<Value, 2>::CreateKDTree(INSERT_NUM);

	std::vector<KDTree<Value, 2>::DataType> vData;
	for(uint32_t i=0; i<INSERT_NUM; ++i)
	{
		KDTree<Value, 2>::DataType item;
		item.Vector[0] = random() % 99;
		item.Vector[1] = random() % 99;
		item.Value.Uin = i;

		vData.push_back(item);
		//printf("new vector (%u, %u)\n", item.Vector[0], item.Vector[1]);
	}

	timeval t1, t2;
	gettimeofday(&t1, NULL);
	kdtree.Build(vData);
	gettimeofday(&t2, NULL);
	printf("const: %.03fs\n", TIME_COST(t1, t2));

	printf("Tree Dump:\n");
	kdtree.DumpTree();

	printf("\n");

	KDTree<Value, 2>::VectorType key;
	key[0] = (random() * time(NULL)) % 99;
	key[1] = (random() * time(NULL)) % 99;

#ifdef KDTREE_USE_DRAW2DMAP
	kdtree.Draw2DMap("map.jpg", key[0], key[1]);
#endif

#define SEARCH_COUNT	5

	uint32_t range = 0;

	KDTree<Value, 2>::DataType buffer[SEARCH_COUNT];
	int foundCount = kdtree.Nearest(key, buffer, SEARCH_COUNT);
	printf("nearest (%u, %u):\n", key[0], key[1]);
	for(int i=0; i<foundCount; ++i)
	{
		uint32_t distance = EuclideanDistance<uint32_t, KDVector<uint32_t, 2>, 2>::Distance(key, buffer[i].Vector);
		if(i == foundCount / 2)
			range = distance;
		printf("  %d:(%u, %u) distance:%u\n", i, buffer[i].Vector[0], buffer[i].Vector[1], distance);
	}

	printf("\n");

	foundCount = kdtree.Range(key, range, buffer, SEARCH_COUNT);
	printf("nearest (%u, %u) with range %u:\n", key[0], key[1], range);
	for(int i=0; i<foundCount; ++i)
	{
		uint32_t distance = EuclideanDistance<uint32_t, KDVector<uint32_t, 2>, 2>::Distance(key, buffer[i].Vector);
		printf("  %d:(%u, %u) distance:%u\n", i, buffer[i].Vector[0], buffer[i].Vector[1], distance);
	}

	printf("\n");

	kdtree.Delete();
	return 0;
}






