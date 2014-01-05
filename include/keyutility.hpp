/*++
 *
 * storage library
 * author: nickeywoo
 * date: 2013.12.19
 *
*--*/
#ifndef __KEYTRANSLATE_HPP__
#define __KEYTRANSLATE_HPP__

#include <utility>
#include <string>
#include <boost/format.hpp>
#include "utility.hpp"

template<typename KeyT>
struct KeySerialization
{
	static std::string Serialization(KeyT key);
};

template<>
struct KeySerialization<int8_t>
{
	static std::string Serialization(int8_t key)
	{
		return (boost::format("%hhd") % key).str();
	}
};

template<>
struct KeySerialization<uint8_t>
{
	static std::string Serialization(uint8_t key)
	{
		return (boost::format("%hhu") % key).str();
	}
};

template<>
struct KeySerialization<int16_t>
{
	static std::string Serialization(int16_t key)
	{
		return (boost::format("%hd") % key).str();
	}
};

template<>
struct KeySerialization<uint16_t>
{
	static std::string Serialization(uint16_t key)
	{
		return (boost::format("%hu") % key).str();
	}
};

template<>
struct KeySerialization<int32_t>
{
	static std::string Serialization(int32_t key)
	{
		return (boost::format("%d") % key).str();
	}
};

template<>
struct KeySerialization<uint32_t>
{
	static std::string Serialization(uint32_t key)
	{
		return (boost::format("%u") % key).str();
	}
};

template<>
struct KeySerialization<int64_t>
{
	static std::string Serialization(int64_t key)
	{
		return (boost::format("%lld") % key).str();
	}
};

template<>
struct KeySerialization<uint64_t>
{
	static std::string Serialization(uint64_t key)
	{
		return (boost::format("%llu") % key).str();
	}
};

template<>
struct KeySerialization<float>
{
	static std::string Serialization(float key)
	{
		return (boost::format("%f") % key).str();
	}
};

template<>
struct KeySerialization<double>
{
	static std::string Serialization(double key)
	{
		return (boost::format("%f") % key).str();
	}
};

template<>
struct KeySerialization<const char*>
{
	static std::string Serialization(const char* key)
	{
		return std::string(key);
	}
};

template<>
struct KeySerialization<std::string>
{
	static std::string Serialization(std::string key)
	{
		return key;
	}
};

template<typename KeyT>
struct KeyCompare
{
	static int Compare(KeyT key1, KeyT key2);
};

template<>
struct KeyCompare<int8_t>
{
	static int Compare(int8_t key1, int8_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<uint8_t>
{
	static int Compare(uint8_t key1, uint8_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<int16_t>
{
	static int Compare(int16_t key1, int16_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<uint16_t>
{
	static int Compare(uint16_t key1, uint16_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<int32_t>
{
	static int Compare(int32_t key1, int32_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<uint32_t>
{
	static int Compare(uint32_t key1, uint32_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<int64_t>
{
	static int Compare(int64_t key1, int64_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<uint64_t>
{
	static int Compare(uint64_t key1, uint64_t key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<float>
{
	static int Compare(float key1, float key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<double>
{
	static int Compare(double key1, double key2)
	{
		return key1 - key2;
	}
};

template<>
struct KeyCompare<const char*>
{
	static int Compare(const char* key1, const char* key2)
	{
		return strcmp(key1, key2);
	}
};

template<>
struct KeyCompare<std::string>
{
	static int Compare(std::string key1, std::string key2)
	{
		return key1.compare(key2);
	}
};

////////////////////////////////////////////////////////////////////
// KeyTranslate
template<typename KeyT>
struct KeyTranslate
{
	typedef KeyT KeyType;
	typedef uint64_t HeadType;

	static inline HeadType Translate(KeyType key)
	{
		return Hash64((const char*)&key, sizeof(KeyType));
	}
};

template<>
struct KeyTranslate<const char*>
{
	typedef const char* KeyType;
	typedef uint64_t HeadType;

	static inline uint64_t Translate(const char* key)
	{
		return Hash64(key, strlen(key));
	}
};

template<>
struct KeyTranslate<std::string>
{
	typedef std::string KeyType;
	typedef uint64_t HeadType;

	static inline uint64_t Translate(std::string& key)
	{
		return Hash64(key.c_str(), key.length());
	}
};

template<>
struct KeyTranslate<int8_t>
{
	typedef int8_t KeyType;
	typedef int8_t HeadType;

	static inline int8_t Translate(int8_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<uint8_t>
{
	typedef uint8_t KeyType;
	typedef uint8_t HeadType;

	static inline uint8_t Translate(uint8_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<int16_t>
{
	typedef int16_t KeyType;
	typedef int16_t HeadType;

	static inline int16_t Translate(int16_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<uint16_t>
{
	typedef uint16_t KeyType;
	typedef uint16_t HeadType;

	static inline uint16_t Translate(uint16_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<int32_t>
{
	typedef int32_t KeyType;
	typedef int32_t HeadType;

	static inline int32_t Translate(int32_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<uint32_t>
{
	typedef uint32_t KeyType;
	typedef uint32_t HeadType;

	static inline uint32_t Translate(uint32_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<int64_t>
{
	typedef int64_t KeyType;
	typedef int64_t HeadType;

	static inline int64_t Translate(int64_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<uint64_t>
{
	typedef uint64_t KeyType;
	typedef uint64_t HeadType;

	static inline uint64_t Translate(uint64_t key)
	{
		return key;
	}
};

template<>
struct KeyTranslate<float>
{
	typedef float KeyType;
	typedef uint32_t HeadType;

	static inline uint32_t Translate(float key)
	{
		return *((uint32_t*)&key);
	}
};

template<>
struct KeyTranslate<double>
{
	typedef double KeyType;
	typedef uint64_t HeadType;

	static inline uint64_t Translate(double key)
	{
		return *((uint64_t*)&key);
	}
};

#endif // define __KEYTRANSLATE_HPP__
