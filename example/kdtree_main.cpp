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

int main(int argc, char* argv[])
{
	KDTree<Value, 2> kdtree = KDTree<Value, 2>::CreateKDTree(20);

	for(uint32_t i=0; i<10; ++i)
	{
		KDTree<Value, 2>::VectorType v;
		v[0] = random() % 99;
		v[1] = random() % 109;

		printf("new vector (%u, %u)\n", v[0], v[1]);

		Value* pValue = kdtree.Hash(v, true);
		if(!pValue)
			break;

		pValue->Uin = i;
	}

	printf("Tree Dump:\n");
	kdtree.DumpTree();

	kdtree.Delete();
	return 0;
}






