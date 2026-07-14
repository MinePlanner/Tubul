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

TEST(PODVectorTest, EmplaceBack) {
    // emplace_back forwards args to T's constructor and returns a reference
    PODVector<Point> v;
    Point& ref = v.emplace_back(1, 2);
    EXPECT_EQ(ref, (Point{1, 2}));
    ref.x = 10;
    EXPECT_EQ(v[0], (Point{10, 2}));

    v.emplace_back(3, 4);
    v.emplace_back(5, 6);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.back(), (Point{5, 6}));

    // self-reference under growth: emplace_back(v[0]) must not read freed memory
    PODVector<int> s = {7};
    for (int i = 0; i < 50; i++) s.emplace_back(s[0]);
    for (auto x : s) EXPECT_EQ(x, 7);
}

TEST(PODVectorTest, Erase) {
    PODVector<int> e = {0, 1, 2, 3, 4, 5};

    // erase(pos) removes one element and returns the iterator to the next
    auto it = e.erase(e.begin() + 1); // remove 1
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(e.size(), 5);
    {
	    std::vector<int> exp = {0, 2, 3, 4, 5};
    	int i = 0;
    	for(auto x : e) EXPECT_EQ(x, exp[i++]);
    }

    // erase(first, last) removes a range and returns the iterator past it
    it = e.erase(e.begin() + 1, e.begin() + 3);   // remove 2, 3
    EXPECT_EQ(*it, 4);
    EXPECT_EQ(e.size(), 3);
    {
	    std::vector<int> exp = {0, 4, 5};
    	int i = 0;
    	for(auto x : e) EXPECT_EQ(x, exp[i++]);
    }

    // erasing the last element (zero-length tail move)
    e.erase(e.end() - 1);
    EXPECT_EQ(e.size(), 2);
    EXPECT_EQ(e.back(), 4);

    // empty range is a no-op that returns the position
    it = e.erase(e.begin(), e.begin());
    EXPECT_EQ(it, e.begin());
    EXPECT_EQ(e.size(), 2);
}

TEST(PODVectorTest, Assign) {
    PODVector<int> a = {1, 2, 3, 4, 5};

    // assign(count, value) replaces the whole content, growing as needed
    a.assign(3, 7);
    EXPECT_EQ(a.size(), 3);
    for (auto x : a) EXPECT_EQ(x, 7);

    a.assign(6, 9);
    EXPECT_EQ(a.size(), 6);
    EXPECT_EQ(a.front(), 9);
    EXPECT_EQ(a.back(), 9);

    a.assign(1, 4);
    EXPECT_EQ(a.size(), 1);
    EXPECT_EQ(a[0], 4);
}

TEST(PODVectorTest, ReverseIteratorsAndFreeSwap) {
    PODVector<int> v = {1, 2, 3, 4};

    // rbegin/rend walk the vector backwards
    std::vector<int> rev(v.rbegin(), v.rend());
    EXPECT_EQ(rev, (std::vector<int>{4, 3, 2, 1}));

    // const reverse iterators and cbegin/cend
    const PODVector<int>& cv = v;
    EXPECT_EQ(*cv.crbegin(), 4);
    EXPECT_EQ(*(cv.crend() - 1), 1);
    EXPECT_EQ((cv.cend() - cv.cbegin()), v.size());

    // explicit non-member TU::swap(a, b)
    PODVector<int> a = {1, 2}, b = {9, 8, 7};
    TU::swap(a, b);
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(b.size(), 2);
    EXPECT_EQ(a[0], 9);
    EXPECT_EQ(b[0], 1);
}

TEST(PODVectorTest, SelfReferenceUnderGrowth) {
    // push_back(v[0]) repeatedly forces many reallocations while the argument
    // aliases an element that grow() is about to move: must not read freed memory
    {
        PODVector<int> v = {7};
        for (int i = 0; i < 50; i++) v.push_back(v[0]);
        EXPECT_EQ(v.size(), 51);
        for (auto x : v) EXPECT_EQ(x, 7);
    }

    // emplace_back(v[0]) has the same hazard
    {
        PODVector<int> v = {9};
        for (int i = 0; i < 50; i++) v.emplace_back(v[0]);
        for (auto x : v) EXPECT_EQ(x, 9);
    }

    // insert(pos, v.back()) exactly at capacity, so the insert triggers growth
    {
        PODVector<int> v = {1, 2, 3, 4};       // init_list reserves size == capacity
        ASSERT_EQ(v.size(), v.capacity());
        v.insert(v.begin(), v.back());
        std::vector<int> exp = {4, 1, 2, 3, 4};
        int i = 0; for (auto x : v) EXPECT_EQ(x, exp[i++]);
    }

    // resize(n, v.back()) growing well past capacity
    {
        PODVector<int> v = {5};
        v.resize(v.capacity() * 8 + 10, v.back());
        for (auto x : v) EXPECT_EQ(x, 5);
    }

    // assign(count, v[0]) where the value aliases an element being cleared
    {
        PODVector<int> v = {42, 1, 2};
        v.assign(10, v[0]);
        EXPECT_EQ(v.size(), 10);
        for (auto x : v) EXPECT_EQ(x, 42);
    }
}

TEST(PODVectorTest, EmptyAndSelfSwap) {
    // copy / move / swap of empty vectors exercise the null-pointer memcpy paths
    PODVector<int> empty;

    PODVector<int> copy = empty;              // copy ctor from empty
    EXPECT_TRUE(copy.empty());

    PODVector<int> assigned = {1, 2, 3};
    assigned = empty;                          // copy assignment from empty
    EXPECT_TRUE(assigned.empty());

    PODVector<int> moved = std::move(empty);   // move ctor from empty
    EXPECT_TRUE(moved.empty());

    PODVector<int> ea, eb;                     // swap of two empties
    ea.swap(eb);
    EXPECT_TRUE(ea.empty());
    EXPECT_TRUE(eb.empty());

    // self-swap must be a no-op, not corruption
    PODVector<int> s = {1, 2, 3};
    s.swap(s);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s[0], 1);
    EXPECT_EQ(s[2], 3);
}
