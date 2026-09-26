#pragma once

#include "defs.h"

#include <algorithm>
#include <array>
#include <functional>
#include <queue>
#include <type_traits>
#include <vector>

template <typename T, typename V, typename D = T>
class KdTree2
{
	static_assert(std::is_pod<V>::value, "The value parameter should be a POD type.");

public:
	using Point = std::array<T, 2>;
	using Value = V;
	using Values = std::vector<Value>;
	using Distance = D;

	KdTree2(std::size_t max_result_size) : max_result_size(max_result_size) {}

	void add(Point const& point, Value value);
	void build();
	void clear();

	// Fills the variable 'result' by all the points in the kdtree
	// which are <= 'distance' away from 'point'
	template <typename IntersectionCheck>
	void search(Point const& point, Distance min, Distance max, Values& result, bool threshold, IntersectionCheck check) const;

protected:
	bool is_ready_for_search = false;
	std::size_t max_result_size;

	// types: empty = -1, split = 0..1, leaf = 2
	// The split value gives the dimension which is used for the split
	using Type = int;
	using KdID = std::size_t;
	struct KdNode
	{
		KdID id = std::numeric_limits<KdID>::max();
		Type type = -1;
		Point point;
		Value value;
#ifdef CERTIFY
		//maintain deletion information for orthogonal range search
		enum class DeleteState { Present, MarkedDeleted, Deleted };
		DeleteState delete_flag = DeleteState::Present;
		bool is_deleted() { return delete_flag == DeleteState::Deleted; }
		bool holds_value() { return delete_flag == DeleteState::Present; }
#endif

		KdNode() = default;
		KdNode(Point const& point, Value value)
			: point(point), value(value) {}

		bool is_empty() const { return type == -1; }
		bool is_inner() const { return type >= 0 && type < 2; }
		bool is_leaf() const { return type == 2; }
	};
	using Tree = std::vector<KdNode>;

	Tree tree;

	// helper structs for the building process
	using TreeIterator = typename Tree::iterator;

	struct BuildElement
	{
		KdID id;
		TreeIterator begin;
		TreeIterator end;

		// BuildElement() = default;
		BuildElement(KdID id, TreeIterator begin, TreeIterator end)
			: id(id), begin(begin), end(end) {}
	};

	struct Comp
	{
		Comp(int dimension) : dimension(dimension) {}

		bool operator()(KdNode const& node1, KdNode const& node2) const
		{
			return node1.point[dimension] < node2.point[dimension];
		};

	private:
		int dimension;
	};

	int calcSplitDimension(TreeIterator begin, TreeIterator end) const;

	static D dist_sqr(Point const& p, Point const& q) {
		return (p[0] - q[0])*(p[0] - q[0]) + (p[1] - q[1])*(p[1] - q[1]);
	}

	class Box {
		D const infty = std::numeric_limits<D>::max();
		using KdTree = KdTree2<T, V, D>;

	public:
		// left, right, bottom, top
		T l, r, b, t;

		Box() : l(-infty), r(infty), b(-infty), t(infty) {}
		Box(T l, T r, T b, T t) : l(l), r(r), b(b), t(t) {}

		D min_distance_sqr(Point const p) {
			bool const in_x_range = (p[0] >= l && p[0] <= r);
			bool const in_y_range = (p[1] >= b && p[1] <= t);

			// check for containment
			if (in_x_range && in_y_range) { return 0; }

			// check for closest point being on the side of the box
			if (in_x_range) {
				if (p[1] < b) { return (b - p[1])*(b - p[1]); }
				if (p[1] > t) { return (p[1] - t)*(p[1] - t); }
				assert(false);
			}
			else if (in_y_range) {
				if (p[0] < l) { return (l - p[0])*(l - p[0]); }
				if (p[0] > r) { return (p[0] - r)*(p[0] - r); }
				assert(false);
			}
			// check for closest point being on the corner of the box
			else  {
				if (p[0] <= l && p[1] <= b) { return KdTree::dist_sqr(p, {l, b}); }
				if (p[0] >= r && p[1] <= b) { return KdTree::dist_sqr(p, {r, b}); }
				if (p[0] <= l && p[1] >= t) { return KdTree::dist_sqr(p, {l, t}); }
				if (p[0] >= r && p[1] >= t) { return KdTree::dist_sqr(p, {r, t}); }
			}
			assert(false);
		}

		D max_distance_sqr(Point const p) {
			auto const center = Point{(l+r)/2., (b+t)/2.};
			if (p[0] <= center[0] && p[1] <= center[1]) { return KdTree::dist_sqr(p, {r, t}); }
			if (p[0] >= center[0] && p[1] <= center[1]) { return KdTree::dist_sqr(p, {l, t}); }
			if (p[0] <= center[0] && p[1] >= center[1]) { return KdTree::dist_sqr(p, {r, b}); }
			if (p[0] >= center[0] && p[1] >= center[1]) { return KdTree::dist_sqr(p, {l, b}); }
			assert(false);
		}
	};
};

template <typename T, typename V, typename D>
void KdTree2<T, V, D>::add(Point const& point, Value value)
{
	tree.emplace_back(point, value);
	is_ready_for_search = false;
}

template <typename T, typename V, typename D>
void KdTree2<T, V, D>::build()
{
	if (tree.empty()) { return; }

	std::vector<BuildElement> build_stack;
	build_stack.emplace_back(0, tree.begin(), tree.end());

	// Give the points the correct IDs (which correspond to their
	// final position in the tree vector)
	while (!build_stack.empty()) {
		auto current = build_stack.back();
		build_stack.pop_back();

		// If only a single element remains in the range
		if (std::distance(current.begin, current.end) == 1) {
			current.begin->id = current.id;
			current.begin->type = 2;
			continue;
		}

		// Find median
		auto median = current.begin + std::distance(current.begin, current.end)/2;
		auto split_dimension = calcSplitDimension(current.begin, current.end);
		std::nth_element(current.begin, median, current.end, Comp(split_dimension));

		// Settle the median element
		median->id = current.id;
		median->type = split_dimension;

		// Find correct IDs of remainng elments (and check for empty range before push)
		if (current.begin < median) {
			build_stack.emplace_back(2*current.id + 1, current.begin, median);
		}
		if (median + 1 < current.end) {
			build_stack.emplace_back(2*current.id + 2, median + 1, current.end);
		}
	}

	// Build tree using the IDs
	// There are 3 steps to this:
	// 1) sort by ID (because that's the order in which the points will appear in the tree vector)
	// 2) resize to largest id (as the tree vector has some gaps)
	// 3) actually place the points at their correct id (starting from the back of the vector)

	// 1)
	auto sort_by_id = [](KdNode const& node1, KdNode const& node2) {
		assert(node1.id != node2.id);
		return node1.id < node2.id;
	};
	std::sort(tree.begin(), tree.end(), sort_by_id);
	// 2)
	auto number_of_points = tree.size();
	tree.resize(tree.back().id + 1);
	// 3)
	for(int i = number_of_points - 1; i >= 0; --i) {
		std::swap(tree[i], tree[tree[i].id]);
	}

	is_ready_for_search = true;
}

template <typename T, typename V, typename D>
int KdTree2<T, V, D>::calcSplitDimension(TreeIterator begin, TreeIterator end) const
{
	Point min;
	Point max;
	min.fill(std::numeric_limits<T>::max());
	max.fill(std::numeric_limits<T>::lowest());

	// find min and max
	auto find_min_max = [&min, &max](KdNode const& node) {
		for (std::size_t i = 0; i < 2; ++i) {
			min[i] = std::min(min[i], node.point[i]);
			max[i] = std::max(max[i], node.point[i]);
		}
	};
	std::for_each(begin, end, find_min_max);

	// find dimension with largest difference
	T max_difference = -1;
	int max_dimension = -1;
	for (int i = 0; i < 2; ++i) {
		if (max[i] - min[i] > max_difference) {
			max_difference = max[i] - min[i];
			max_dimension = i;
		}
	}

	assert(max_dimension != -1 && max_difference >= 0);
	return max_dimension;
}

template <typename T, typename V, typename D>
void KdTree2<T, V, D>::clear()
{
	tree.clear();
	is_ready_for_search = false;
}


template <typename T, typename V, typename D>
template <typename IntersectionCheck>
void KdTree2<T, V, D>::search(Point const& query_point, Distance min, Distance max, Values& result, bool threshold, IntersectionCheck check) const
{
	assert(is_ready_for_search);

	if (threshold && result.size() >= max_result_size) { return; }

	struct QueueElement {
		KdID id;
		Box box;
	};
	std::queue<QueueElement> search_queue;
	search_queue.push({0, Box()});

	auto const max_sqr = max*max;
	auto const min_sqr = min*min;
	while (!search_queue.empty()) {
		auto const current_element = search_queue.front();
		auto const& kd_point = tree[current_element.id];
		search_queue.pop();
		if (kd_point.is_empty()) { continue; }

		// Check if this point is in range
		auto const dist_sqr = this->dist_sqr(kd_point.point, query_point);
		if (dist_sqr >= min_sqr && dist_sqr <= max_sqr) {
			if (check({kd_point.point[0], kd_point.point[1]})) {
				result.push_back(kd_point.value);
				if (threshold && result.size() >= max_result_size) { return; }
			}
		}
		if (kd_point.is_leaf()) { continue; }

		// Search in subtrees
		assert(kd_point.is_inner());

		auto const dimension = kd_point.type;
		// auto const query_coord = query_point[dimension];
		auto const split_coord = kd_point.point[dimension];

		auto child_box1 = current_element.box;
		auto child_box2 = current_element.box;
		if (dimension == 0) {
			child_box1.r = split_coord;
			child_box2.l = split_coord;
		}
		else {
			child_box1.t = split_coord;
			child_box2.b = split_coord;
		}

		auto const max1 = child_box1.max_distance_sqr(query_point);
		auto const min1 = child_box1.min_distance_sqr(query_point);
		if (min1 <= max_sqr && max1 >= min_sqr) {
			assert(2*current_element.id + 1 < tree.size());
			search_queue.push({2*current_element.id + 1, child_box1});
		}

		auto const max2 = child_box2.max_distance_sqr(query_point);
		auto const min2 = child_box2.min_distance_sqr(query_point);
		if (2*current_element.id + 2 < tree.size() && min2 <= max_sqr && max2 >= min_sqr) {
			search_queue.push({2*current_element.id + 2, child_box2});
		}
	}
}
