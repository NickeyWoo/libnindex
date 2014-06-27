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
#include "btree.hpp"

struct Key {
	uint32_t Type;
	uint16_t Score;
	time_t Timestamp;
};

struct Value {
	uint32_t Uin;
	char Name[36];
	uint8_t Age;
	uint8_t Gender;
};

int main(int argc, char* argv[])
{
	printf("degree: %u\n", AutoDegree<Key>::Degree);
	printf("sizeof(Key): %lu\n", sizeof(Key));
	printf("sizeof(BTreeIndexNode): %lu\n", sizeof(BTreeIndexNode<Key, AutoDegree<Key>::Degree>));
	printf("sizeof(BTreeLeafNode): %lu\n", sizeof(BTreeLeafNode<Key, Value, AutoDegree<Key>::Degree>));
	return 0;
}


