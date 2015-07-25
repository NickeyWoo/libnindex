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

#define BITMAP_MAGIC    "BITMAP@@"
#define BITMAP_VERSION  0x0101

template<typename HeadT>
struct BitmapMeta {
    char cMagic[8];
    uint16_t wVersion;

    uint64_t ddwMemSize;
    uint32_t dwHeadSize;

    uint64_t ddwSeed;
    uint64_t ddwUsed;

    uint32_t dwReserved[4];

    HeadT Head;
} __attribute__((packed));
template<>
struct BitmapMeta<void> {
    char cMagic[8];
    uint16_t wVersion;

    uint64_t ddwMemSize;
    uint32_t dwHeadSize;

    uint64_t ddwSeed;
    uint64_t ddwUsed;

    uint32_t dwReserved[4];
} __attribute__((packed));

template<typename KeyT, typename HeadT = void>
class Bitmap
{
public:
	static Bitmap<KeyT, HeadT> CreateBitmap(size_t bitCount)
	{
        size_t size = sizeof(BitmapMeta<HeadT>) + bitCount / 8; 
        if(bitCount % 8 != 0)
            ++size;

		Bitmap<KeyT, HeadT> bitmap;
        bitmap.m_BitmapMetaInfo = (BitmapMeta<HeadT>*)malloc(size);
        if(bitmap.m_BitmapMetaInfo == NULL)
            return bitmap;

		memset(bitmap.m_BitmapMetaInfo, 0, size);

        memcpy(bitmap.m_BitmapMetaInfo->cMagic, BITMAP_MAGIC, 8);
        bitmap.m_BitmapMetaInfo->wVersion = BITMAP_VERSION;

        bitmap.m_BitmapMetaInfo->ddwMemSize = size;
        bitmap.m_BitmapMetaInfo->dwHeadSize = sizeof(BitmapMeta<HeadT>);

        bitmap.m_BitmapMetaInfo->ddwSeed = bitCount;
        bitmap.m_BitmapMetaInfo->ddwUsed = 0;

		bitmap.m_BitmapBuffer = (char*)bitmap.m_BitmapMetaInfo + sizeof(BitmapMeta<HeadT>);
		bitmap.m_NeedDelete = true;
		return bitmap;
	}

	static Bitmap<KeyT, HeadT> LoadBitmap(char* buffer, size_t size)
	{
		Bitmap<KeyT, HeadT> bitmap;
        if(size <= sizeof(BitmapMeta<HeadT>))
            return bitmap;

		bitmap.m_BitmapMetaInfo = (BitmapMeta<HeadT>*)buffer;
        if(memcmp(bitmap.m_BitmapMetaInfo->cMagic, "\0\0\0\0\0\0\0\0", 8) == 0)
        {
            memcpy(bitmap.m_BitmapMetaInfo->cMagic, BITMAP_MAGIC, 8);
            bitmap.m_BitmapMetaInfo->wVersion = BITMAP_VERSION;

            bitmap.m_BitmapMetaInfo->ddwMemSize = size;
            bitmap.m_BitmapMetaInfo->dwHeadSize = sizeof(BitmapMeta<HeadT>);

            bitmap.m_BitmapMetaInfo->ddwSeed = (size - sizeof(BitmapMeta<HeadT>)) * 8;
            bitmap.m_BitmapMetaInfo->ddwUsed = 0;

            bitmap.m_BitmapBuffer = (char*)bitmap.m_BitmapMetaInfo + sizeof(BitmapMeta<HeadT>);
        }
        else
        {
            if(memcmp(bitmap.m_BitmapMetaInfo->cMagic, BITMAP_MAGIC, 8) != 0 ||
                bitmap.m_BitmapMetaInfo->wVersion != BITMAP_VERSION ||
                bitmap.m_BitmapMetaInfo->ddwMemSize != size ||
                bitmap.m_BitmapMetaInfo->dwHeadSize != sizeof(BitmapMeta<HeadT>))
            {
                bitmap.m_BitmapMetaInfo = NULL;
                return bitmap;
            }

            bitmap.m_BitmapBuffer = (char*)bitmap.m_BitmapMetaInfo + sizeof(BitmapMeta<HeadT>);
        }
		return bitmap;
	}

	template<typename StorageT>
	static Bitmap<KeyT, HeadT> LoadBitmap(StorageT storage)
	{
		return Bitmap<KeyT, HeadT>::LoadBitmap(storage.GetStorageBuffer(), storage.GetSize());
	}

    static size_t GetBufferSize(size_t bitCount)
    {
        size_t size = bitCount / 8 + sizeof(BitmapMeta<HeadT>);
        if(bitCount % 8 != 0)
            ++size;
        return size;
    }

    inline bool Success()
    {
        return m_BitmapMetaInfo != NULL && m_BitmapBuffer != NULL;
    }

    inline char* GetBuffer()
    {
        return m_BitmapBuffer;
    }

    HeadT* GetHead()
    {
        if(m_BitmapMetaInfo == NULL)
            return NULL;
        return &m_BitmapMetaInfo->Head;
    }

	void Delete()
	{
		if(m_NeedDelete && m_BitmapMetaInfo)
			free(m_BitmapMetaInfo);
        m_BitmapMetaInfo = NULL;
        m_BitmapBuffer = NULL;
	}

	bool Set(KeyT key)
	{
		if(m_BitmapMetaInfo == NULL || m_BitmapBuffer == NULL)
			return false;

		size_t hash = KeyTranslate<KeyT>::Translate(key) % m_BitmapMetaInfo->ddwSeed;
		size_t pos = hash / 8;
		size_t offset = hash % 8;
		size_t op = m_BitmapBuffer[pos] & (0x1 << offset);

		m_BitmapBuffer[pos] |= 0x1 << offset;

        if(!op) ++m_BitmapMetaInfo->ddwUsed;
		return op?true:false;
	}

	bool Unset(KeyT key)
	{
		if(m_BitmapMetaInfo == NULL || m_BitmapBuffer == NULL)
			return false;

		size_t hash = KeyTranslate<KeyT>::Translate(key) % m_BitmapMetaInfo->ddwSeed;
		size_t pos = hash / 8;
		size_t offset = hash % 8;
		size_t op = m_BitmapBuffer[pos] & (0x1 << offset);

		m_BitmapBuffer[pos] &= ~(0x1 << offset);

        if(op) --m_BitmapMetaInfo->ddwUsed;
		return op?true:false;
	}

	bool Contains(KeyT key)
	{
		if(m_BitmapMetaInfo == NULL || m_BitmapBuffer == NULL)
			return false;

		size_t hash = KeyTranslate<KeyT>::Translate(key) % m_BitmapMetaInfo->ddwSeed;
		size_t pos = hash / 8;
		size_t offset = hash % 8;
		size_t op = m_BitmapBuffer[pos] & (0x1 << offset);

		return op?true:false;
	}

    float Capacity()
    {
		if(m_BitmapMetaInfo == NULL || m_BitmapBuffer == NULL)
            return 1;
        return (float)m_BitmapMetaInfo->ddwUsed / m_BitmapMetaInfo->ddwSeed;
    }

	void Dump()
	{
		HexDump((char*)m_BitmapMetaInfo, m_BitmapMetaInfo->ddwMemSize, NULL);
	}

	Bitmap() :
		m_NeedDelete(false),
		m_BitmapMetaInfo(NULL),
		m_BitmapBuffer(NULL)
	{
	}

protected:
	bool m_NeedDelete;

    BitmapMeta<HeadT>* m_BitmapMetaInfo;
	char* m_BitmapBuffer;
};

#endif // define __BITMAP_HPP__
