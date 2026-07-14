#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>
#include <string>
#include <memory>
#include <ranges>

using TU::SmallVector;

namespace {

struct SVPoint {
    int x, y;
    bool operator==(const SVPoint& other) const {
        return x == other.x and y == other.y;
    }
};

// Non-trivially-copyable type that counts alive instances, to catch missing
// destructor calls, double destructions and leaks.
struct Tracked {
    static inline int live = 0;
    std::string s;

    Tracked() { ++live; }
    explicit Tracked(std::string v) : s(std::move(v)) { ++live; }
    Tracked(const Tracked& o) : s(o.s) { ++live; }
    Tracked(Tracked&& o) noexcept : s(std::move(o.s)) { ++live; }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) noexcept = default;
    ~Tracked() { --live; }
    bool operator==(const Tracked& o) const { return s == o.s; }
};
static_assert(!std::is_trivially_copyable_v<Tracked>);

// Non-trivially-copyable but safe to relocate with memcpy (it owns memory but
// holds no pointer into itself).
struct RelocInt {
    int* p;
    RelocInt() : p(new int(0)) {}
    explicit RelocInt(int v) : p(new int(v)) {}
    RelocInt(const RelocInt& o) : p(new int(*o.p)) {}
    RelocInt(RelocInt&& o) noexcept : p(std::exchange(o.p, nullptr)) {}
    RelocInt& operator=(const RelocInt& o) { *this = RelocInt(o); return *this; }
    RelocInt& operator=(RelocInt&& o) noexcept { std::swap(p, o.p); return *this; }
    ~RelocInt() { delete p; }
};
static_assert(!std::is_trivially_copyable_v<RelocInt>);

// long enough to defeat the small string optimization, so every element drags
// a real heap allocation
std::string bigstr(int i) {
    return "payload-that-does-not-fit-in-sso-" + std::to_string(i);
}

} // namespace

template <>
inline constexpr bool TU::detail::is_relocatable_v<RelocInt> = true;

TEST(SmallVectorTest, testSmallVectorBasics) {
    // Create an empty vec
    SmallVector<int, 4> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 4);
    EXPECT_TRUE(vec.is_small());

    // Push elements and confirm the new items, size and capacity
    for(size_t i = 0; i < 4; i++) vec.push_back(i);
    for(size_t i = 0; i < 4; i++) EXPECT_EQ(vec[i], i);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_TRUE(vec.is_small());
    EXPECT_EQ(vec.capacity(), 4);

    // Front and back working
    EXPECT_EQ(vec.front(), 0);
    EXPECT_EQ(vec.back(), 3);

    // Buffer changes from stack to capacity when the capacity is not enough
    vec.push_back(4);
    EXPECT_FALSE(vec.is_small());
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec.capacity(), 6); // new_capacity = 4 + 4/2 = 6

    // Expect the new capacity with 1.5 factor
    vec.push_back(5);
    vec.push_back(6);
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec.capacity(), 9); // new_capacity = 6 + 6/2 = 9

    // Capacity doesn't change with pop_backs
    // the vector shouldn't be small because the capacity is still 9
    for(size_t i = 0; i < 3; i++) vec.pop_back();
    EXPECT_EQ(vec.size(), 4);
    EXPECT_FALSE(vec.is_small());
    EXPECT_EQ(vec.capacity(), 9);

    // The clear is working
    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_FALSE(vec.is_small());
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


    // memset doesnt work on every type, but the resize will work
    SmallVector<int, 4> mem_vec;
    // 5 will be interpreted as 0x05 and transformed to 05|05|05|05
    std::memset(mem_vec.data(), 5, 4 * sizeof(4));
    for(size_t i = 0; i < 4; i++) EXPECT_EQ(mem_vec[i], 84215045);

    mem_vec.resize(4, 5);
    for(size_t i = 0; i < 4; i++) EXPECT_EQ(mem_vec[i], 5);


    SmallVector<SVPoint, 2> point_vec;
    point_vec.push_back({1, 2});
    point_vec.push_back({3, 4});
    point_vec.push_back({5, 6});   // it should grow
    ASSERT_EQ(point_vec.size(), 3);
    EXPECT_EQ(point_vec[0], (SVPoint{1, 2}));
    EXPECT_EQ(point_vec[2], (SVPoint{5, 6}));

    // sizeof SmallVector<int, 4> is the same as a SmallVector<SVPoint, 2> because both of them have 4 ints
    EXPECT_EQ(sizeof(SmallVector<int, 4>), sizeof(SmallVector<SVPoint, 2>));
}


TEST(SmallVectorTest, CopyingAndMoving) {
    // copying as a copy and not pointer in the stack
    SmallVector<int, 4> a = {1, 2, 3};
    SmallVector<int, 4> b = a;
    b[0] = 1000;
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(b[0], 1000);

    // copying as a copy in the heap
    SmallVector<int, 4> c = {1, 2, 3, 4, 5};
    SmallVector<int, 4> d = c;
    d[0] = 1000;
    EXPECT_EQ(c[0], 1);
    EXPECT_EQ(d[0], 1000);

    // copying overwrites
    SmallVector<int, 4> e = {1, 2, 3, 4, 5};
    SmallVector<int, 4> f = {0, 0};
    EXPECT_EQ(f.size(), 2);
    f = e;
    EXPECT_EQ(f.size(), e.size());
    for(size_t i = 0; i < e.size(); i++) EXPECT_EQ(f[i], e[i]);

    // swapping between small vectors
    // a = {1, 2, 3}, b = {1000, 2, 3}
    a.swap(b);
    EXPECT_EQ(a[0], 1000);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(b.size(), 3);

    // swapping between heap vectors
    e[0] = 1000;
    e.push_back(6);
    e.swap(f);
    EXPECT_EQ(f[0], 1000);
    EXPECT_EQ(f[5], 6);
    EXPECT_EQ(e[0], 1);
    EXPECT_FALSE(e.is_small());
    EXPECT_FALSE(f.is_small());

    // swapping between heap and stack vectors
    a[0] = 1;
    // a = {1, 2, 3}, f = {1000, 2, 3, 4, 5, 6}
    a.swap(f);
    EXPECT_TRUE(f.is_small());
    EXPECT_FALSE(a.is_small());
    EXPECT_EQ(a.size(), 6);
    EXPECT_EQ(f.size(), 3);
    EXPECT_EQ(a[0], 1000);
    EXPECT_EQ(a[5], 6);
    EXPECT_EQ(f[0], 1);

    // swapping back to check the inverse swap
    f.swap(a);
    EXPECT_TRUE(a.is_small());
    EXPECT_FALSE(f.is_small());
    EXPECT_EQ(f.size(), 6);
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(f[0], 1000);
    EXPECT_EQ(f[5], 6);
    EXPECT_EQ(a[0], 1);
}


TEST(SmallVectorTest, IteratorsAndRanges) {
    SmallVector<int, 4> sorted_vec = {1, 100, 105, 200, 200, 300};
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


    SmallVector<int, 4> v{1, 2, 3};
    EXPECT_EQ((v.end() - v.begin()), v.size());

    auto doublevec = sorted_vec | std::ranges::views::transform([](int x) { return 2 * x;});
    ind = 0;
    for(auto x: doublevec) EXPECT_EQ(2 * real_vector[ind++], x);
}

TEST(SmallVectorTest, InsertElements) {
    // Inserting at a random position, the begin and the end
    {
        SmallVector<int, 4> v = {1, 2, 4, 5};
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
        SmallVector<int, 2> v = {1, 3};
        EXPECT_TRUE(v.is_small());

        v.insert(v.begin() + 1, 2);
        EXPECT_FALSE(v.is_small());
        EXPECT_EQ(v.size(), 3);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[1], 2);
        EXPECT_EQ(v[2], 3);
    }

    // Inserting iterators
    {
        SmallVector<int, 4> v = {1, 2, 6, 7};
        std::vector<int> src = {3, 4, 5};

        auto it = v.insert(v.begin() + 2, src.begin(), src.end());
        EXPECT_EQ(*it, 3);
        EXPECT_EQ(v.size(), 7);
        std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7};
        int ind = 0;
        for(auto& x : v) EXPECT_EQ(x, expected[ind++]);
    }
}

// Non-trivially_copyable

TEST(SmallVectorTest, NonPodBasics) {
    SmallVector<std::string, 4> vec;
    EXPECT_TRUE(vec.is_small());

    for (int i = 0; i < 10; i++) vec.push_back(bigstr(i));
    EXPECT_FALSE(vec.is_small());
    EXPECT_EQ(vec.size(), 10);
    for (int i = 0; i < 10; i++) EXPECT_EQ(vec[i], bigstr(i));

    // resize down destroys, resize up fills
    vec.resize(3);
    EXPECT_EQ(vec.size(), 3);
    vec.resize(6, bigstr(999));
    for (size_t i = 3; i < 6; i++) EXPECT_EQ(vec[i], bigstr(999));

    // insert in the middle, at begin and at end, growing from small
    SmallVector<std::string, 2> v = {bigstr(1), bigstr(3)};
    v.insert(v.begin() + 1, bigstr(2));
    v.insert(v.begin(), bigstr(0));
    v.insert(v.end(), bigstr(4));
    ASSERT_EQ(v.size(), 5);
    for (int i = 0; i < 5; i++) EXPECT_EQ(v[i], bigstr(i));

    // range insert in the middle without growing past it
    std::vector<std::string> src = {bigstr(10), bigstr(11)};
    v.insert(v.begin() + 2, src.begin(), src.end());
    ASSERT_EQ(v.size(), 7);
    EXPECT_EQ(v[1], bigstr(1));
    EXPECT_EQ(v[2], bigstr(10));
    EXPECT_EQ(v[3], bigstr(11));
    EXPECT_EQ(v[4], bigstr(2));

    // count insert
    v.insert(v.begin(), 3, bigstr(777));
    ASSERT_EQ(v.size(), 10);
    for (int i = 0; i < 3; i++) EXPECT_EQ(v[i], bigstr(777));
    EXPECT_EQ(v[3], bigstr(0));
}

TEST(SmallVectorTest, NonPodLifetimes) {
    Tracked::live = 0;
    {
        SmallVector<Tracked, 2> vec;
        for (int i = 0; i < 8; i++) vec.push_back(Tracked(bigstr(i)));
        EXPECT_EQ(Tracked::live, 8);

        vec.pop_back();
        EXPECT_EQ(Tracked::live, 7);

        vec.resize(3);
        EXPECT_EQ(Tracked::live, 3);

        vec.resize(5, Tracked(bigstr(50)));
        EXPECT_EQ(Tracked::live, 5);

        vec.insert(vec.begin() + 1, 2, Tracked(bigstr(60)));
        EXPECT_EQ(Tracked::live, 7);
        EXPECT_EQ(vec[0].s, bigstr(0));
        EXPECT_EQ(vec[1].s, bigstr(60));
        EXPECT_EQ(vec[2].s, bigstr(60));
        EXPECT_EQ(vec[3].s, bigstr(1));

        vec.clear();
        EXPECT_EQ(Tracked::live, 0);
        EXPECT_FALSE(vec.is_small());
    }
    EXPECT_EQ(Tracked::live, 0);

    // copies duplicate lifetimes, moves transfer them
    {
        SmallVector<Tracked, 2> a;
        for (int i = 0; i < 5; i++) a.push_back(Tracked(bigstr(i)));
        EXPECT_EQ(Tracked::live, 5);

        SmallVector<Tracked, 2> b = a;
        EXPECT_EQ(Tracked::live, 10);
        EXPECT_EQ(b[4].s, bigstr(4));

        SmallVector<Tracked, 2> c = std::move(a);
        EXPECT_EQ(Tracked::live, 10);
        EXPECT_EQ(c.size(), 5);
        EXPECT_EQ(a.size(), 0);

        b = std::move(c);
        EXPECT_EQ(Tracked::live, 5);
        EXPECT_EQ(b.size(), 5);
        EXPECT_EQ(b[4].s, bigstr(4));

        // copy assignment overwrites previous contents
        SmallVector<Tracked, 2> d;
        d.push_back(Tracked(bigstr(100)));
        d = b;
        EXPECT_EQ(Tracked::live, 10);
        EXPECT_EQ(d.size(), 5);
    }
    EXPECT_EQ(Tracked::live, 0);

    // swaps in every storage combination
    {
        SmallVector<Tracked, 4> small1 = {Tracked(bigstr(1))};
        SmallVector<Tracked, 4> small2 = {Tracked(bigstr(2)), Tracked(bigstr(3))};
        small1.swap(small2);
        EXPECT_EQ(small1.size(), 2);
        EXPECT_EQ(small2.size(), 1);
        EXPECT_EQ(small1[0].s, bigstr(2));
        EXPECT_EQ(small2[0].s, bigstr(1));

        SmallVector<Tracked, 2> heap1;
        for (int i = 0; i < 5; i++) heap1.push_back(Tracked(bigstr(i)));
        SmallVector<Tracked, 2> heap2;
        for (int i = 10; i < 14; i++) heap2.push_back(Tracked(bigstr(i)));
        heap1.swap(heap2);
        EXPECT_EQ(heap1.size(), 4);
        EXPECT_EQ(heap2.size(), 5);
        EXPECT_EQ(heap1[0].s, bigstr(10));
        EXPECT_EQ(heap2[0].s, bigstr(0));

        SmallVector<Tracked, 2> mixed_small = {Tracked(bigstr(1))};
        mixed_small.swap(heap1);
        EXPECT_EQ(mixed_small.size(), 4);
        EXPECT_FALSE(mixed_small.is_small());
        EXPECT_EQ(heap1.size(), 1);
        EXPECT_TRUE(heap1.is_small());
        EXPECT_EQ(heap1[0].s, bigstr(1));
        EXPECT_EQ(mixed_small[0].s, bigstr(10));
    }
    EXPECT_EQ(Tracked::live, 0);
}

TEST(SmallVectorTest, MoveOnlyTypes) {
    SmallVector<std::unique_ptr<int>, 2> vec;
    for (int i = 0; i < 6; i++) vec.push_back(std::make_unique<int>(i));
    ASSERT_EQ(vec.size(), 6);
    for (int i = 0; i < 6; i++) EXPECT_EQ(*vec[i], i);

    vec.insert(vec.begin() + 3, std::make_unique<int>(100));
    ASSERT_EQ(vec.size(), 7);
    EXPECT_EQ(*vec[2], 2);
    EXPECT_EQ(*vec[3], 100);
    EXPECT_EQ(*vec[4], 3);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 6);

    SmallVector<std::unique_ptr<int>, 2> moved = std::move(vec);
    EXPECT_EQ(moved.size(), 6);
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(*moved[3], 100);

    moved.clear();
    EXPECT_TRUE(moved.empty());
}

TEST(SmallVectorTest, RelocatableOptIn) {
    static_assert(TU::detail::is_relocatable_v<RelocInt>);

    // growth for an opt-in relocatable type goes through the realloc/memcpy
    // path: a double delete or a lost element would blow up here
    SmallVector<RelocInt, 2> vec;
    for (int i = 0; i < 20; i++) vec.push_back(RelocInt(i));
    ASSERT_EQ(vec.size(), 20);
    for (int i = 0; i < 20; i++) EXPECT_EQ(*vec[i].p, i);

    vec.insert(vec.begin() + 5, RelocInt(500));
    EXPECT_EQ(*vec[5].p, 500);
    EXPECT_EQ(*vec[6].p, 5);

    vec.resize(4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(*vec[3].p, 3);
}

TEST(SmallVectorTest, EmplaceBack) {
    // emplace_back forwards args and returns a reference, small and after growth
    SmallVector<SVPoint, 2> p;
    SVPoint& ref = p.emplace_back(1, 2); // parenthesized aggregate init (C++20)
    EXPECT_EQ(ref, (SVPoint{1, 2}));
    p.emplace_back(3, 4);
    p.emplace_back(5, 6); // grows to heap
    EXPECT_FALSE(p.is_small());
    EXPECT_EQ(p.size(), 3);
    EXPECT_EQ(p.back(), (SVPoint{5, 6}));

    // non-trivial element type
    SmallVector<std::string, 2> s;
    std::string& sref = s.emplace_back(bigstr(0));
    EXPECT_EQ(sref, bigstr(0));
    for(int i = 1; i < 6; i++) s.emplace_back(bigstr(i));   // forces growth
    EXPECT_EQ(s.size(), 6);
    for(int i = 0; i < 6; i++) EXPECT_EQ(s[i], bigstr(i));
}

TEST(SmallVectorTest, EraseAssignReverseSwap) {
    // erase on a trivially copyable type (memmove path), living on the heap
    SmallVector<int, 4> e = {0, 1, 2, 3, 4, 5};
    auto it = e.erase(e.begin() + 1);  // remove 1
    EXPECT_EQ(*it, 2);
    {
	    std::vector<int> exp = {0, 2, 3, 4, 5};
    	int i = 0;
    	for(auto x : e) EXPECT_EQ(x, exp[i++]);
    }
    it = e.erase(e.begin() + 1, e.begin() + 3);   // remove 2, 3
    EXPECT_EQ(*it, 4);
    {
	    std::vector<int> exp = {0, 4, 5};
    	int i = 0;
    	for(auto x : e) EXPECT_EQ(x, exp[i++]);
    }
    e.erase(e.end() - 1);
    EXPECT_EQ(e.size(), 2);
    EXPECT_EQ(e.back(), 4);

    // assign(count, value), growing past the inline buffer and shrinking back
    SmallVector<int, 4> a;
    a.assign(5, 7);
    EXPECT_EQ(a.size(), 5);
    EXPECT_FALSE(a.is_small());
    for(auto x : a) EXPECT_EQ(x, 7);
    a.assign(2, 3);
    EXPECT_EQ(a.size(), 2);
    EXPECT_EQ(a[0], 3);
    EXPECT_EQ(a[1], 3);

    // reverse iterators and cbegin/cend
    SmallVector<int, 4> v = {1, 2, 3, 4};
    std::vector<int> rev(v.rbegin(), v.rend());
    EXPECT_EQ(rev, (std::vector<int>{4, 3, 2, 1}));
    const SmallVector<int, 4>& cv = v;
    EXPECT_EQ(*cv.crbegin(), 4);
    EXPECT_EQ((cv.cend() - cv.cbegin()), v.size());

    // explicit non-member TU::swap(a, b)
    SmallVector<int, 4> x = {1, 2}, y = {9, 8, 7};
    TU::swap(x, y);
    EXPECT_EQ(x.size(), 3);
    EXPECT_EQ(y.size(), 2);
    EXPECT_EQ(x[0], 9);
    EXPECT_EQ(y[0], 1);
}

TEST(SmallVectorTest, EraseAssignLifetimes) {
    // erase/assign must destroy exactly the right elements for non-trivial types
    Tracked::live = 0;
    {
        SmallVector<Tracked, 2> v;
        for(int i = 0; i < 6; i++) v.emplace_back(bigstr(i));   // heap, live 6
        EXPECT_EQ(Tracked::live, 6);

        // erase(pos): moves the tail down and destroys one element
        auto it = v.erase(v.begin() + 1);      // remove element 1
        EXPECT_EQ(Tracked::live, 5);
        EXPECT_EQ(it->s, bigstr(2));
        EXPECT_EQ(v[0].s, bigstr(0));
        EXPECT_EQ(v[1].s, bigstr(2));

        // erase(first, last): destroys two elements
        v.erase(v.begin() + 1, v.begin() + 3); // remove 2, 3
        EXPECT_EQ(Tracked::live, 3);
        EXPECT_EQ(v[1].s, bigstr(4));

        // assign replaces all previous content (old ones destroyed)
        v.assign(2, Tracked(bigstr(99)));
        EXPECT_EQ(Tracked::live, 2);
        EXPECT_EQ(v[0].s, bigstr(99));
        EXPECT_EQ(v[1].s, bigstr(99));
    }
    EXPECT_EQ(Tracked::live, 0);
}

TEST(SmallVectorTest, SelfReferenceUnderGrowth) {
    // trivially copyable: push_back/emplace_back/insert/resize/assign where the
    // argument aliases an element the container may relocate during growth
    {
        SmallVector<int, 2> v = {7};
        for (int i = 0; i < 50; i++) v.push_back(v[0]);   // crosses inline -> heap
        EXPECT_FALSE(v.is_small());
        for (auto x : v) EXPECT_EQ(x, 7);
    }
    {
        SmallVector<int, 2> v = {9};
        for (int i = 0; i < 50; i++) v.emplace_back(v[0]);
        for (auto x : v) EXPECT_EQ(x, 9);
    }
    {
        SmallVector<int, 4> v = {1, 2, 3, 4};    // small and exactly full
        v.insert(v.begin(), v.back());            // triggers growth to heap
        std::vector<int> exp = {4, 1, 2, 3, 4};
        int i = 0; for (auto x : v) EXPECT_EQ(x, exp[i++]);
    }
    {
        SmallVector<int, 2> v = {5};
        v.resize(100, v.back());
        for (auto x : v) EXPECT_EQ(x, 5);
    }
    {
        SmallVector<int, 2> v = {42, 1, 2};
        v.assign(10, v[0]);
        EXPECT_EQ(v.size(), 10);
        for (auto x : v) EXPECT_EQ(x, 42);
    }

    // non-trivial: emplace_back(v[0]) and assign(n, v[0]) must not use-after-free
    // (this is the heap-use-after-free the assign fix closed)
    Tracked::live = 0;
    {
        SmallVector<Tracked, 2> v;
        v.emplace_back(bigstr(0));
        for (int i = 0; i < 20; i++) v.emplace_back(v[0]);   // self-ref across growth
        EXPECT_EQ(v.size(), 21);
        for (const auto& t : v) EXPECT_EQ(t.s, bigstr(0));

        v.assign(5, v[0]);                                    // self-ref in assign
        EXPECT_EQ(v.size(), 5);
        for (const auto& t : v) EXPECT_EQ(t.s, bigstr(0));
    }
    EXPECT_EQ(Tracked::live, 0);
}

TEST(SmallVectorTest, EmptyAndSelfSwap) {
    // copy / move / swap of empty vectors
    SmallVector<int, 4> empty;

    SmallVector<int, 4> copy = empty;
    EXPECT_TRUE(copy.empty());

    SmallVector<int, 4> assigned = {1, 2, 3};
    assigned = empty;
    EXPECT_TRUE(assigned.empty());

    SmallVector<int, 4> moved = std::move(empty);
    EXPECT_TRUE(moved.empty());

    SmallVector<int, 4> ea, eb;
    ea.swap(eb);
    EXPECT_TRUE(ea.empty());
    EXPECT_TRUE(eb.empty());

    // self-swap must be a no-op on inline storage (guards the fix in swap)
    SmallVector<int, 4> s = {1, 2, 3};
    EXPECT_TRUE(s.is_small());
    s.swap(s);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s[0], 1);
    EXPECT_EQ(s[2], 3);

    // and on heap storage
    SmallVector<int, 2> h = {1, 2, 3, 4, 5};
    EXPECT_FALSE(h.is_small());
    h.swap(h);
    EXPECT_EQ(h.size(), 5);
    EXPECT_EQ(h[0], 1);
    EXPECT_EQ(h[4], 5);
}
