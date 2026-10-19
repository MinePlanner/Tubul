#include <gtest/gtest.h>
#include "tubul.h"
#include "tubul_blocks.h"
#include <string>
#include <thread>
#include <vector>

TEST(Blocks, nestPerThreadAndShareTheStats)
{
	constexpr size_t THREADS = 8;
	constexpr size_t REPETITIONS = 1000;

	std::vector<std::string> locations(THREADS);
	std::vector<std::thread> threads;
	for (size_t t = 0; t < THREADS; ++t)
		threads.emplace_back([t, &locations] {
			for (size_t i = 0; i < REPETITIONS; ++i) {
				TU::Block outer("concurrentOuter", TU::Block::LogType::NONE);
				TU::Block inner("concurrentInner", TU::Block::LogType::NONE);
				if (i == 0)
					locations[t] = TU::getCurrentBlockLocation();
			}
		});
	for (auto& thread : threads)
		thread.join();

	// Each thread sees only its own blocks.
	for (const auto& location : locations)
		EXPECT_EQ(location, "concurrentOuter.concurrentInner");
	EXPECT_EQ(TU::getCurrentBlockLocation(), "");

	// Every closing is counted once.
	EXPECT_EQ(TU::getAccumulatedStats("concurrentOuter").count_, THREADS * REPETITIONS);
	EXPECT_EQ(TU::getAccumulatedStats("concurrentInner").count_, THREADS * REPETITIONS);
}
