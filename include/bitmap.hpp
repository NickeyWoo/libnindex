/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.19
 *
*--*/
#ifndef __BITMAP_HPP__
#define __BITMAP_HPP__

#include <utility>
#include <string>
#include <time.h>
#include "utility.hpp"
#include "keyutility.hpp"
#include "storage.hpp"

template<typename KeyT>
class Bitmap
{
public:
	static Bitmap<KeyT> CreateBitmap(size_t size)
	{
		Bitmap<KeyT> bitmap;
		bitmap.m_NeedDelete = true;

		bitmap.m_BufferSize = size;
		bitmap.m_Seed = GetPrime(size * 8);

		bitmap.m_BitmapBuffer = (char*)malloc(size);
		memset(bitmap.m_BitmapBuffer, 0, size);
		return bitmap;
	}

	static Bitmap<KeyT> LoadBitmap(char* buffer, size_t size)
	{
		Bitmap<KeyT> bitmap;
		bitmap.m_BitmapBuffer = buffer;
		bitmap.m_BufferSize = size;
		bitmap.m_Seed = GetPrime(size * 8);
		return bitmap;
	}

	template<typename StorageT>
	static Bitmap<KeyT> LoadBitmap(StorageT storage)
	{
		Bitmap<KeyT> bitmap;
		bitmap.m_BitmapBuffer = storage.GetBuffer();
		bitmap.m_BufferSize = storage.GetSize();
		bitmap.m_Seed = GetPrime(bitmap.m_BufferSize * 8);
		return bitmap;
	}

	void Delete()
	{
		if(m_NeedDelete && m_BitmapBuffer)
		{
			free(m_BitmapBuffer);
			m_BitmapBuffer = NULL;
		}
	}

	bool Set(KeyT key)
	{
		if(m_BitmapBuffer == NULL)
			return false;

		size_t hash = KeyTranslate<KeyT>::Translate(key) % m_Seed;
		size_t pos = hash / 8;
		size_t offset = hash % 8;
		size_t op = m_BitmapBuffer[pos] & (0x1 << offset);

		m_BitmapBuffer[pos] |= 0x1 << offset;

		return op?true:false;
	}

	bool Unset(KeyT key)
	{
		if(m_BitmapBuffer == NULL)
			return false;

		size_t hash = KeyTranslate<KeyT>::Translate(key) % m_Seed;
		size_t pos = hash / 8;
		size_t offset = hash % 8;
		size_t op = m_BitmapBuffer[pos] & (0x1 << offset);

		m_BitmapBuffer[pos] &= ~(0x1 << offset);

		return op?true:false;
	}

	bool Contains(KeyT key)
	{
		if(m_BitmapBuffer == NULL)
			return false;

		size_t hash = KeyTranslate<KeyT>::Translate(key) % m_Seed;
		size_t pos = hash / 8;
		size_t offset = hash % 8;
		size_t op = m_BitmapBuffer[pos] & (0x1 << offset);

		return op?true:false;
	}

	void Dump()
	{
		HexDump(m_BitmapBuffer, m_BufferSize, NULL);
	}

	Bitmap() :
		m_NeedDelete(false),
		m_Seed(0),
		m_BitmapBuffer(NULL),
		m_BufferSize(0)
	{
	}

protected:
	bool m_NeedDelete;

	size_t m_Seed;

	char* m_BitmapBuffer;
	size_t m_BufferSize;
};

#endif // define __BITMAP_HPP__
