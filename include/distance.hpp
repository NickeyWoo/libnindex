/*++
 *
 * nindex library
 * author: nickeywoo
 * date: 2014.01.10
 *
*--*/
#ifndef __DISTANCE_HPP__
#define __DISTANCE_HPP__

#include <utility>
#include <vector>
#include <math.h>

template<typename KeyT, typename VectorType, uint8_t DimensionValue>
struct EuclideanDistance
{
	static KeyT Distance(VectorType& key1, VectorType& key2)
	{
		KeyT ret = 0;
		for(uint8_t i=0; i<DimensionValue; ++i)
		{
			ret += (key1[i] - key2[i]) * (key1[i] - key2[i]);
		}
		return ret;
	}
};

template<typename KeyT, typename VectorType, uint8_t DimensionValue>
struct ManhattanDistance
{
	static KeyT Distance(VectorType& key1, VectorType& key2)
	{
		KeyT ret = 0;
		for(uint8_t i=0; i<DimensionValue; ++i)
		{
			ret += (key2[i] > key1[i])?(key2[i] - key1[i]):(key1[i] - key2[i]);
		}
		return ret;
	}
};

#endif // define __DISTANCE_HPP__
