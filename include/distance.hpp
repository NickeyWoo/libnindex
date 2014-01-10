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

template<typename KeyT, uint8_t Dimension>
struct NodeVector;

template<typename KeyT, uint8_t Dimension>
struct EuclideanDistance
{
	static double Distance(NodeVector<KeyT, Dimension> key1, NodeVector<KeyT, Dimension> key2)
	{
		double ret;
		for(uint8_t i=0; i<Dimension; ++i)
		{
			ret += pow(key1[i] - key2[i], 2);
		}
		return sqrt(ret);
	}
};

template<typename KeyT, uint8_t Dimension>
struct ManhattanDistance
{
	static double Distance(NodeVector<KeyT, Dimension> key1, NodeVector<KeyT, Dimension> key2)
	{
		double ret;
		for(uint8_t i=0; i<Dimension; ++i)
		{
			ret += fabs(key1[i] - key2[i]);
		}
		return ret;
	}
};

#endif // define __DISTANCE_HPP__
