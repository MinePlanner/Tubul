#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>
#include <ranges>

struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x and y == other.y;
    }
};

using TU::PODVector;

TEST(PODVectorTest, testPODVectorBasics) {
    // Create an empty vec: no inline storage, so it owns nothing yet
    PODVector<int> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);

    // Push elements and confirm the new items and size. With no SBO the buffer
    // grows on every push until it has room to apply the 1.5 factor: 0,1,2,3,4
    for(size_t i = 0; i < 4; i++) vec.push_back(i);
    for(size_t i = 0; i < 4; i++) EXPECT_EQ(vec[i], i);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec.capacity(), 4);

    // Front and back working
    EXPECT_EQ(vec.front(), 0);
    EXPECT_EQ(vec.back(), 3);

    // Growth follows the 1.5 factor once there is capacity to grow from
    vec.push_back(4);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec.capacity(), 6); // next_capacity = 4 + 4/2 = 6

    vec.push_back(5);
    vec.push_back(6);
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec.capacity(), 9); // next_capacity = 6 + 6/2 = 9

    // Capacity doesn't change with pop_backs
    for(size_t i = 0; i < 3; i++) vec.pop_back();
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec.capacity(), 9);

    // The clear keeps the capacity
    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.capacity(), 9);

    // Pop back with 0 elements does nothing
    vec.pop_back();
    EXPECT_TRUE(vec.empty());

    // Out of range with at
    EXPECT_THROW(vec.at(0), std::out_of_range);
    EXPECT_THROW(vec.at(1), std::out_of_range);

    // Resize and fill with default value
    vec = {1, 2, 3, 4, 5, 6, 7};
    EXPECT_EQ(vec.size(), 7);
    vec.resize(2);
    EXPECT_EQ(vec.size(), 2);
    vec.resize(5, 100);
    for(size_t i = 2; i < 5; i++) EXPECT_EQ(vec[i], 100);


    // memset doesn't work on every type, but it does on the raw POD storage we
    // own after reserving it (size stays 0, the bytes are ours to write)
    PODVector<int> mem_vec;
    mem_vec.reserve(4);
    // 5 will be interpreted as 0x05 and transformed to 05|05|05|05
    std::memset(mem_vec.data(), 5, 4 * sizeof(int));
    for(size_t i = 0; i < 4; i++) EXPECT_EQ(mem_vec[i], 84215045);

    mem_vec.resize(4, 5);
    for(size_t i = 0; i < 4; i++) EXPECT_EQ(mem_vec[i], 5);


    PODVector<Point> point_vec;
    point_vec.push_back({1, 2});
    point_vec.push_back({3, 4});
    point_vec.push_back({5, 6});   // it should grow
    ASSERT_EQ(point_vec.size(), 3u);
    EXPECT_EQ(point_vec[0], (Point{1, 2}));
    EXPECT_EQ(point_vec[2], (Point{5, 6}));

    // sizeof is independent of T: every PODVector is just {size, capacity, ptr}
    EXPECT_EQ(sizeof(PODVector<int>), sizeof(PODVector<Point>));
}


TEST(PODVectorTest, CopyingAndMoving) {
    // copying makes an independent deep copy
    PODVector<int> a = {1, 2, 3};
    PODVector<int> b = a;
    b[0] = 1000;
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(b[0], 1000);

    // copying a larger vector
    PODVector<int> c = {1, 2, 3, 4, 5};
    PODVector<int> d = c;
    d[0] = 1000;
    EXPECT_EQ(c[0], 1);
    EXPECT_EQ(d[0], 1000);

    // copy assignment overwrites
    PODVector<int> e = {1, 2, 3, 4, 5};
    PODVector<int> f = {0, 0};
    EXPECT_EQ(f.size(), 2);
    f = e;
    EXPECT_EQ(f.size(), e.size());
    for(size_t i = 0; i < e.size(); i++) EXPECT_EQ(f[i], e[i]);

    // swapping small vectors
    // a = {1, 2, 3}, b = {1000, 2, 3}
    a.swap(b);
    EXPECT_EQ(a[0], 1000);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(b.size(), 3);

    // swapping larger vectors
    e[0] = 1000;
    e.push_back(6);
    e.swap(f);
    EXPECT_EQ(f[0], 1000);
    EXPECT_EQ(f[5], 6);
    EXPECT_EQ(e[0], 1);

    // swapping vectors of different sizes
    a[0] = 1;
    // a = {1, 2, 3}, f = {1000, 2, 3, 4, 5, 6}
    a.swap(f);
    EXPECT_EQ(a.size(), 6);
    EXPECT_EQ(f.size(), 3);
    EXPECT_EQ(a[0], 1000);
    EXPECT_EQ(a[5], 6);
    EXPECT_EQ(f[0], 1);

    // swapping back to check the inverse swap
    f.swap(a);
    EXPECT_EQ(f.size(), 6);
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(f[0], 1000);
    EXPECT_EQ(f[5], 6);
    EXPECT_EQ(a[0], 1);
}


TEST(PODVectorTest, MovePreservesPointers) {
	// as we only work in the heap we can move the data
	// without losing the pointers information.
    PODVector<int> src = {10, 20, 30, 40, 50};
    int* p = &src[2];

    PODVector<int> dst = std::move(src);
    EXPECT_EQ(*p, 30);
    EXPECT_EQ(dst[2], 30);
    EXPECT_EQ(src.size(), 0);

    // move assignment behaves the same
    PODVector<int> other = {1, 2};
    other = std::move(dst);
    EXPECT_EQ(*p, 30);
    EXPECT_EQ(other.size(), 5);
    EXPECT_EQ(dst.size(), 0);
}


TEST(PODVectorTest, IteratorsAndRanges) {
    PODVector<int> sorted_vec = {1, 100, 105, 200, 200, 300};
    EXPECT_EQ(*std::lower_bound(sorted_vec.begin(), sorted_vec.end(), 1), 1);
    EXPECT_EQ(*std::upper_bound(sorted_vec.begin(), sorted_vec.end(), 1), 100);
    EXPECT_EQ(*std::lower_bound(sorted_vec.begin(), sorted_vec.end(), 50), 100);
    EXPECT_EQ(*std::lower_bound(sorted_vec.begin(), sorted_vec.end(), 100), 100);
    EXPECT_EQ(*std::lower_bound(sorted_vec.begin(), sorted_vec.end(), 103), 105);

    // auto
    std::vector<int> real_vector = {1, 100, 105, 200, 200, 300};
    int ind = 0;
    for(auto& x : sorted_vec)
        EXPECT_EQ(x, real_vector[ind++]);


    PODVector<int> v{1, 2, 3};
    EXPECT_EQ((v.end() - v.begin()), v.size());

    auto doublepodvec = sorted_vec | std::ranges::views::transform([](int x) { return 2 * x;});
    ind = 0;
    for(auto x: doublepodvec) EXPECT_EQ(2 * real_vector[ind++], x);
}

TEST(PODVectorTest, InsertElements) {
    // Inserting at a random position, the begin and the end
    {
        PODVector<int> v = {1, 2, 4, 5};
        auto it = v.insert(v.begin() + 2, 3);
        EXPECT_EQ(*it, 3);
        EXPECT_EQ(v.size(), 5);


        auto it_begin = v.insert(v.begin(), 0);
        EXPECT_EQ(*it_begin, 0);
        EXPECT_EQ(v.front(), 0);

        auto it_end = v.insert(v.end(), 6);
        EXPECT_EQ(*it_end, 6);
        EXPECT_EQ(v.back(), 6);

        std::vector<int> expected = {0, 1, 2, 3, 4, 5, 6};
        int ind = 0;
        for(auto& x : v) EXPECT_EQ(x, expected[ind++]);
    }

    // Checking growth and small condition
    {
        PODVector<int> v = {1, 3};
        v.insert(v.begin() + 1, 2);
        EXPECT_EQ(v.size(), 3);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[1], 2);
        EXPECT_EQ(v[2], 3);
    }

    // Inserting iterators
    {
        PODVector<int> v = {1, 2, 6, 7};
        std::vector<int> src = {3, 4, 5};

        auto it = v.insert(v.begin() + 2, src.begin(), src.end());
        EXPECT_EQ(*it, 3);
        EXPECT_EQ(v.size(), 7);
        std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7};
        int ind = 0;
        for(auto& x : v) EXPECT_EQ(x, expected[ind++]);
    }
}
