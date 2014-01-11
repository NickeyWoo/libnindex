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
#include "kdtree.hpp"

struct Pepole
{
	uint32_t Uin;
};

#define INSERT_NUM 	20

int main(int argc, char* argv[])
{
	KDTree<Pepole, 2> kdtree = KDTree<Pepole, 2>::CreateKDTree(INSERT_NUM);

	KDTree<Pepole, 2>::ImportDataType * pBuffer = (KDTree<Pepole, 2>::ImportDataType*)malloc(sizeof(KDTree<Pepole, 2>::ImportDataType) * INSERT_NUM);
	memset(pBuffer, 0, sizeof(KDTree<Pepole, 2>::ImportDataType) * INSERT_NUM);
	for(int i=0; i<INSERT_NUM; ++i)
	{
		pBuffer[i].Vector[0] = random() % 19;
		pBuffer[i].Vector[1] = random() % 19;

		pBuffer[i].Value.Uin = random() % 10000;
	}

	if(kdtree.OptimumImport(pBuffer, INSERT_NUM) < 0)
	{
		printf("error: optimum import data fail.\n");
		return -1;
	}

	kdtree.DumpTree();

	kdtree.Delete();
	return 0;
}






