#pragma once

#include "geometry_basics.h"
#include "layer_queue.h"

struct SearchBox
{
	Point min;
	Point max;

	SearchBox(Point const& min, Point const& max)
		: min(min), max(max) {}

	bool empty() const { return min.x >= max.x || min.y >= max.y; }
	Point center() const { return {(min.x + max.x)/2, (min.y + max.y)/2}; }
	// XXX: inefficient to return min/max by value...
	Point bottomLeft() const { return min; }
	Point bottomRight() const { return {max.x, min.y}; }
	Point topLeft() const { return {min.x, max.y}; }
	Point topRight() const { return max; }

	distance_t getDiagonalDist() const { return min.dist(max); }

	bool contains(Point const& point) const
	{
		return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
	}
};
using SearchBoxes = LayerQueue<SearchBox>;

struct SearchBoxWithMin
{
	Point min;
	Point max;
	distance_t min_dist;
	std::size_t layer;

	SearchBoxWithMin(Point const& min, Point const& max, distance_t const min_dist, std::size_t layer)
		: min(min), max(max), min_dist(min_dist), layer(layer) {}

	operator SearchBox() const {
		return {min, max};
	}

	bool operator>(SearchBoxWithMin const& other) const {
		return min_dist > other.min_dist;
	}

	bool empty() const { return min.x >= max.x || min.y >= max.y; }
	Point center() const { return {(min.x + max.x)/2, (min.y + max.y)/2}; }
	// XXX: inefficient to return min/max by value...
	Point bottomLeft() const { return min; }
	Point bottomRight() const { return {max.x, min.y}; }
	Point topLeft() const { return {min.x, max.y}; }
	Point topRight() const { return max; }

	distance_t getDiagonalDist() const { return min.dist(max); }

	bool contains(Point const& point) const
	{
		return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
	}
};
using SearchBoxesWithMin = LayerQueue<SearchBoxWithMin>;
