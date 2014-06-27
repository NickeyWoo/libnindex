/*++
 *
 *
 *
 *
*--*/
#ifndef __BLOCKFILE_HPP__
#define __BLOCKFILE_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "utility.hpp"

#ifndef DEFAULT_BLOCKSIZE
	#define DEFAULT_BLOCKSIZE	4096
#endif

struct BlockFileHead
{
	char Magic[16];
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	off_t FreeList;
};

template<size_t BlockSize = DEFAULT_BLOCKSIZE>
class BlockFile
{
public:
	static int Open(BlockFile* pstFile, const char* szFile)
	{
		if(pstFile == NULL)
			return -1;

		pstFile->m_fd = open(szFile, O_RDWR, 0666);
		if(pstFile->m_fd == -1)
		{
			pstFile->m_fd = open(szFile, O_RDWR|O_CREAT, 0666);
			if(pstFile->m_fd == -1)
				return -1;

		}

		return 0;
	}

	void Close()
	{
		close(m_fd);
	}

	void Flush()
	{
		fsync(m_fd);
	}

	off_t AllocBlock(size_t size)
	{
	}

	void ReleaseBlock(off_t pBlock)
	{
	}

private:
	int m_fd;
};

#endif // define __BLOCKFILE_HPP__
