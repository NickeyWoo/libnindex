/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.19
 *
*--*/
#ifndef __STORAGE_HPP__
#define __STORAGE_HPP__

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

class FileStorage
{
public:
	static int OpenStorage(FileStorage* pStorage, const char* szFile, size_t size, off_t pos = 0);
	void Release();

	FileStorage();
	void Flush();

	inline char* GetStorageBuffer()
	{
		return m_Buffer;
	}

	inline size_t GetSize()
	{
		return m_BufferSize;
	}

private:
	int m_fd;
	char* m_Buffer;
	size_t m_BufferSize;
};

class SharedMemoryStorage
{
public:
	static int OpenStorage(SharedMemoryStorage* pStorage, key_t shmKey, size_t size);
	void Release();

	SharedMemoryStorage();
	
	inline char* GetStorageBuffer()
	{
		return m_Buffer;
	}
	
	inline size_t GetSize()
	{
		return m_BufferSize;
	}

private:
	int m_shmid;
	char* m_Buffer;
	size_t m_BufferSize;
};


#endif // define __STORAGE_HPP__
