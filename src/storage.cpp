#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#define FILESTORAGE_DEFAULT_WRITE_BUFFERSIZE		4096

int FileStorage::OpenStorage(FileStorage* pStorage, const char* szFile, size_t size, off_t pos)
{
	if(pStorage == NULL)
		return -1;

	pStorage->m_fd = open(szFile, O_RDWR|O_CREAT, 0666);
	if(pStorage->m_fd == -1)
		return -1;

	struct stat fileStat;
	memset(&fileStat, 0, sizeof(struct stat));
	if(fstat(pStorage->m_fd, &fileStat) < -1)
	{
		pStorage->m_Buffer = NULL;
		close(pStorage->m_fd);
		return -1;
	}

	size_t needSize = pos + size;
	if((size_t)fileStat.st_size < needSize)
	{
		if(lseek(pStorage->m_fd, (needSize - fileStat.st_size - 1), SEEK_END) == -1)
		{
			pStorage->m_Buffer = NULL;
			close(pStorage->m_fd);
			return -1;
		}
		if(write(pStorage->m_fd, "\0", 1) == -1)
		{
			pStorage->m_Buffer = NULL;
			close(pStorage->m_fd);
			return -1;
		}
	}

	pStorage->m_Buffer = (char*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, pStorage->m_fd, pos);
	if(pStorage->m_Buffer == MAP_FAILED)
	{
		pStorage->m_Buffer = NULL;
		close(pStorage->m_fd);
		return -1;
	}
	close(pStorage->m_fd);
	pStorage->m_BufferSize = size;
	return 0;
}

void FileStorage::Flush()
{
	msync(m_Buffer, m_BufferSize, MS_SYNC);
}

void FileStorage::Release()
{
	Flush();
	munmap(m_Buffer, m_BufferSize);
}

FileStorage::FileStorage() :
	m_fd(-1),
	m_Buffer(NULL),
	m_BufferSize(0)
{
}

int SharedMemoryStorage::OpenStorage(SharedMemoryStorage* pStorage, key_t shmKey, size_t size)
{
	if(pStorage == NULL)
		return -1;

	pStorage->m_BufferSize = size;

	pStorage->m_shmid = shmget(shmKey, 0, SHM_W|SHM_R);
	if(pStorage->m_shmid < 0)
	{
		pStorage->m_shmid = shmget(shmKey, pStorage->m_BufferSize, IPC_CREAT|SHM_W|SHM_R);
		if(pStorage->m_shmid < 0)
			return -1;
	}

	pStorage->m_Buffer = (char*)shmat(pStorage->m_shmid, NULL, 0);
	if(pStorage->m_Buffer == (char*)-1)
	{
		pStorage->m_Buffer = NULL;
		return -1;
	}
	return 0;
}

void SharedMemoryStorage::Release()
{
	if(m_Buffer != NULL)
	{
		shmdt(m_Buffer);
		m_Buffer = NULL;
	}
}

SharedMemoryStorage::SharedMemoryStorage() :
	m_shmid(-1),
	m_Buffer(NULL),
	m_BufferSize(0)
{
}


