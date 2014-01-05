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

int main(int argc, char* argv[])
{
	BloomFilter<std::string> bf = BloomFilter<std::string>::CreateBloomFilter(4096, 16);

	uint32_t i=0;
	for(; i<5000; ++i)
	{
		std::string strUrl = (boost::format("http://voanews.com/article/%u") % i).str();
		if(bf.Contains(strUrl))
			break;

		printf("crawl url[%s]...\n", strUrl.c_str());
		bf.Add(strUrl);
	}

	printf("set count: %u\n", i);

	// dump bloomfilter bitmap buffer
	bf.Dump();

	bf.Delete();
	return 0;
}




