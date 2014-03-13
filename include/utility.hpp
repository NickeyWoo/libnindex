/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2013.12.19
 *
*--*/
#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#include <utility>

struct NullType;
struct EmptyType { };

template<typename Type1, typename Type2>
struct TypeList
{
	typedef Type1 HeadType;
	typedef Type2 TailType;
};

#define TYPELIST_1(T1) 										TypeList<T1, NullType>
#define TYPELIST_2(T1, T2) 									TypeList<T1, TYPELIST_1(T2) >
#define TYPELIST_3(T1, T2, T3)								TypeList<T1, TYPELIST_2(T2, T3) >
#define TYPELIST_4(T1, T2, T3, T4) 							TypeList<T1, TYPELIST_3(T2, T3, T4) >
#define TYPELIST_5(T1, T2, T3, T4, T5) 						TypeList<T1, TYPELIST_4(T2, T3, T4, T5) >
#define TYPELIST_6(T1, T2, T3, T4, T5, T6) 					TypeList<T1, TYPELIST_5(T2, T3, T4, T5, T6) >
#define TYPELIST_7(T1, T2, T3, T4, T5, T6, T7) 				TypeList<T1, TYPELIST_6(T2, T3, T4, T5, T6, T7) >
#define TYPELIST_8(T1, T2, T3, T4, T5, T6, T7, T8) 			TypeList<T1, TYPELIST_7(T2, T3, T4, T5, T6, T7, T8) >
#define TYPELIST_9(T1, T2, T3, T4, T5, T6, T7, T8, T9) 		TypeList<T1, TYPELIST_8(T2, T3, T4, T5, T6, T7, T8, T9) >

template<typename TypeListT> struct TypeListLength;
template<> 
struct TypeListLength<NullType>
{
	enum { Length = 0 };
};
template<typename Type1, typename Type2> 
struct TypeListLength< TypeList<Type1, Type2> >
{
	enum { Length = 1 + TypeListLength<Type2>::Length };
};

template<typename TypeListT, uint32_t Index> struct TypeListAt;
template<typename Type1, typename Type2>
struct TypeListAt<TypeList<Type1, Type2>, 0>
{
	typedef Type1 ResultType;
};
template<typename Type1, typename Type2, uint32_t Index>
struct TypeListAt<TypeList<Type1, Type2>, Index>
{
	typedef typename TypeListAt<Type2, Index - 1>::ResultType ResultType;
};

template<typename TypeListT, typename T> struct TypeListIndexOf;
template<typename T>
struct TypeListIndexOf<NullType, T>
{
	enum { Index = -1 };
};
template<typename Type2, typename T>
struct TypeListIndexOf<TypeList<T, Type2>, T>
{
	enum { Index = 0 };
};
template<typename Type1, typename Type2, typename T>
struct TypeListIndexOf<TypeList<Type1, Type2>, T>
{
private:
	enum { temp = TypeListIndexOf<Type2, T>::Index };

public:
	enum { Index = (temp == -1)?-1:(1 + temp) };
};

template<typename T, typename F>
struct alias_cast_t {
	union {
		T raw;
		F data;
	};
};
template<typename T, typename F>
T alias_cast(F rawdata)
{
	alias_cast_t<T, F> ac;
	ac.raw = rawdata;
	return ac.data;
}

#ifndef ntohll
	#define ntohll(val)	\
			((uint64_t)ntohl(0xFFFFFFFF&val) << 32 | ntohl((0xFFFFFFFF00000000&val) >> 32))
#endif

typedef std::pair<uint64_t, uint64_t> uint128_t;

uint32_t Hash32(const char* ptr, size_t len);
uint64_t Hash64(const char* ptr, size_t len);
uint128_t Hash128(const char* ptr, size_t len);

uint32_t MD5_32(const char* ptr, size_t len);
uint64_t MD5_64(const char* ptr, size_t len);
uint128_t MD5_128(const char* ptr, size_t len);

void HexDump(const char* ptr, size_t len, char** out);

size_t GetPrime(size_t num);
size_t GetPrimes(size_t num, size_t* buffer, size_t len);

class Seed
{
public:
	size_t GetCount();
	void Release();

	inline size_t* GetSeedBuffer()
	{
		return m_Buffer;
	}

	inline size_t GetSize()
	{
		return m_BufferSize;
	}

	Seed(size_t seed, size_t count);

private:
	size_t* m_Buffer;
	size_t m_BufferSize;
};

#endif // define __UTILITY_HPP__
