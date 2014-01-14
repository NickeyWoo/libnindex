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

template<typename KeyT, uint8_t Dimensions>
struct NodeVector;

template<typename KeyT, uint8_t Dimensions>
struct EuclideanDistance
{
	static double Distance(const NodeVector<KeyT, Dimensions>& key1, const NodeVector<KeyT, Dimensions>& key2)
	{
		double ret;
		for(uint8_t i=0; i<Dimensions; ++i)
		{
			ret += (key1[i] - key2[i]) * (key1[i] - key2[i]);
		}
		return sqrt(ret);
	}
};

template<typename KeyT, uint8_t Dimensions>
struct ManhattanDistance
{
	static double Distance(const NodeVector<KeyT, Dimensions>& key1, const NodeVector<KeyT, Dimensions>& key2)
	{
		double ret;
		for(uint8_t i=0; i<Dimensions; ++i)
		{
			ret += fabs(key1[i] - key2[i]);
		}
		return ret;
	}
};

#endif // define __DISTANCE_HPP__
