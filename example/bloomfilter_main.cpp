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
#include "bitmap.hpp"
#include "bloomfilter.hpp"
#include "hashtable.hpp"

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        printf("usage: bloomfilter [num] [error]\n");
        return 0;
    }

    uint32_t dwNum = strtoul(argv[1], NULL, 10);
    double dError = strtod(argv[2], NULL) / 100;

	size_t size = BloomFilter<std::string>::GetBufferSize(dwNum, dError);
	size_t k = BloomFilter<std::string>::GetK(dwNum, dError);
	BloomFilter<std::string> bf = BloomFilter<std::string>::CreateBloomFilter(size, k);

	uint32_t success = 0;
	for(uint32_t i=0; i<dwNum; ++i)
	{
		std::string strUrl = (boost::format("http://voanews.com/article/%u") % i).str();
		if(bf.Contains(strUrl))
			break;

		++success;
		//printf("crawl url[%s]...\n", strUrl.c_str());
		bf.Add(strUrl);
	}

    printf("size:%lu, k:%lu\n", size, k);
    printf("capacity: %.02f%%\n", bf.Capacity() * 100);
	printf("success count(%.02f%%): %u, %u\n", (float)success * 100 / dwNum, success, dwNum);

	// dump bloomfilter bitmap buffer
	// bf.Dump();

	bf.Delete();
	return 0;
}




