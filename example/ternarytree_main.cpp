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

int main(int argc, char* argv[])
{
	FileStorage fs;
	if(FileStorage::OpenStorage(&fs, "TernaryTree.index", TernaryTree<char>::GetBufferSize(100, 10)) < 0)
	{
		printf("error: open file fail.\n");
		return -1;
	}

	TernaryTree<char> tt = TernaryTree<char>::LoadTernaryTree(fs, 100, 10);

	fs.Flush();
	fs.Release();
	return 0;
}

