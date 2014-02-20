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

#define INSERT_NUM 1000

int main(int argc, char* argv[])
{
	size_t size = BloomFilter<std::string>::GetBufferSize(INSERT_NUM, 0.001);
	size_t k = BloomFilter<std::string>::GetK(INSERT_NUM, 0.001);
	BloomFilter<std::string> bf = BloomFilter<std::string>::CreateBloomFilter(size, k);

	double success = 0;
	for(uint32_t i=0; i<INSERT_NUM; ++i)
	{
		std::string strUrl = (boost::format("http://voanews.com/article/%u") % i).str();
		if(bf.Contains(strUrl))
			break;

		++success;
		//printf("crawl url[%s]...\n", strUrl.c_str());
		bf.Add(strUrl);
	}

	printf("success count: %u[size:%lu, k:%lu]\n", (uint32_t)success, size, k);

	// dump bloomfilter bitmap buffer
	// bf.Dump();

	bf.Delete();
	return 0;
}




