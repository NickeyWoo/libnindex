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
#include "utility.hpp"
#include "storage.hpp"
#include "bitmap.hpp"

struct Key
{
	uint64_t	ddwUserId;
	char		sAttributeName[64];
};

int main(int argc, char* argv[])
{
	Bitmap<int> bm = Bitmap<int>::CreateBitmap(4096);

	int i=0;
	for(; i<5000; ++i)
	{
		// random key
		Key key;
		memset(&key, 0, sizeof(Key));
		key.ddwUserId = random();
		sprintf(key.sAttributeName, "%lu", random());

		if(bm.Contains(i))
			break;

		bm.Set(i);
	}

	printf("set count: %d\n", i);
	printf("capacity: %.02f%%\n", bm.Capacity() * 100);

	// dump bitmap buffer
	bm.Dump();

	bm.Delete();
	return 0;
}




