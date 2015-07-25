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

class MapStorage
{
public:
	static int OpenStorage(MapStorage* pStorage, const char* szFile, size_t size, off_t pos = 0);
	void Release();

	MapStorage();
	void Flush(int flags = MS_ASYNC);

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

class FileStorage
{
public:
	static int OpenStorage(FileStorage* pStorage, size_t size);
    void Release();

	FileStorage();
	int Flush(const char* szFile);
		
	inline char* GetStorageBuffer()
	{
		return m_Buffer;
	}
	
	inline size_t GetSize()
	{
		return m_BufferSize;
	}

private:
	char* m_Buffer;
	size_t m_BufferSize;

};

#endif // define __STORAGE_HPP__
