#include "tubul.h"
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

// Tests of the iteration order and the thread safety that
// tubul_flat_hash_map.h promises for TU::FlatHashMap and TU::FlatHashSet.

namespace
{

using Map = TU::FlatHashMap<uint64_t, uint64_t>;
using Set = TU::FlatHashSet<uint64_t>;

uint64_t keyOf(const Map::value_type &element)
{
	return element.first;
}

uint64_t keyOf(uint64_t key)
{
	return key;
}

void insert(Map &map, uint64_t key)
{
	map[key] = key;
}

void insert(Set &set, uint64_t key)
{
	set.insert(key);
}

// The keys of a container in the order of its iteration.
template <class Container>
std::vector<uint64_t> iterationOrder(const Container &container)
{
	std::vector<uint64_t> keys;
	for (const auto &element : container)
		keys.push_back(keyOf(element));
	return keys;
}

// The same insertions, erasures and reserve() on a new container.
template <class Container>
std::vector<uint64_t> orderAfterOperations()
{
	Container container;
	for (uint64_t key = 0; key < 1000; ++key)
		insert(container, key * 7919);
	for (uint64_t key = 0; key < 1000; key += 3)
		container.erase(key * 7919);
	container.reserve(5000);
	for (uint64_t key = 1000; key < 1500; ++key)
		insert(container, key * 7919);
	return iterationOrder(container);
}

// Runs check(thread) in several threads at once.
template <class Check>
void runInThreads(Check check)
{
	std::vector<std::thread> threads;
	for (int thread = 0; thread < 8; ++thread)
		threads.emplace_back(check, thread);
	for (std::thread &thread : threads)
		thread.join();
}

} // namespace

// ---------------------------------------------------------------------------
// Iteration order
// ---------------------------------------------------------------------------

// Two containers that get the same operations iterate in the same order.
TEST(FlatHashOrderTest, SameOperationsGiveTheSameOrder)
{
	EXPECT_EQ(orderAfterOperations<Map>(), orderAfterOperations<Map>());
	EXPECT_EQ(orderAfterOperations<Set>(), orderAfterOperations<Set>());
}

// The order doesn't depend on where the container is in memory: the same
// operations after other allocations give the same order.
TEST(FlatHashOrderTest, OrderDoesNotDependOnTheAddress)
{
	const std::vector<uint64_t>          mapOrder = orderAfterOperations<Map>();
	const std::vector<uint64_t>          setOrder = orderAfterOperations<Set>();
	std::vector<std::unique_ptr<char[]>> clutter;
	for (size_t size = 1; size < 100000; size *= 3)
		clutter.push_back(std::make_unique<char[]>(size));
	EXPECT_EQ(orderAfterOperations<Map>(), mapOrder);
	EXPECT_EQ(orderAfterOperations<Set>(), setOrder);
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

// Const member functions run at the same time on one container: lookups and
// iteration in every thread find what is there.
TEST(FlatHashThreadsTest, ConstCallsRunAtTheSameTime)
{
	Map map;
	Set set;
	for (uint64_t key = 0; key < 10000; ++key)
	{
		insert(map, key);
		insert(set, key);
	}
	const Map       &constMap = map;
	const Set       &constSet = set;
	std::vector<int> failures(8, 0);
	runInThreads([&](int thread)
				 {
		for (uint64_t key = 0; key < 20000; ++key)
		{
			const bool expected = key < 10000;
			if (constMap.contains(key) != expected || constSet.contains(key) != expected)
				++failures[thread];
			const auto it = constMap.find(key);
			if (expected && (it == constMap.end() || it->second != key))
				++failures[thread];
		}
		if (iterationOrder(constMap).size() != 10000 || iterationOrder(constSet).size() != 10000)
			++failures[thread]; });
	for (int count : failures)
		EXPECT_EQ(count, 0);
}

// Different containers are used in different threads at the same time,
// including empty ones, which share no state that is written.
TEST(FlatHashThreadsTest, DifferentContainersInEachThread)
{
	std::vector<int> failures(8, 0);
	runInThreads([&](int thread)
				 {
		for (int round = 0; round < 50; ++round)
		{
			Map map;
			Set set;
			if (map.contains(1) || set.contains(1) || map.begin() != map.end())
				++failures[thread];
			for (uint64_t key = 0; key < 500; ++key)
			{
				insert(map, key + thread);
				insert(set, key + thread);
			}
			for (uint64_t key = 0; key < 500; key += 2)
			{
				map.erase(key + thread);
				set.erase(key + thread);
			}
			if (map.size() != 250 || set.size() != 250)
				++failures[thread];
		} });
	for (int count : failures)
		EXPECT_EQ(count, 0);
}
