//
// Created by Carlos Acosta on 15-02-24.
//
#include <gtest/gtest.h>
#include <random>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>
#include "tubul.h"

//Super simple structure used in tests.
struct SampleStruct {
    int a; int b;
    double c; char d;
};
//Common function to test a common pattern.
//Lifetime allocations should increase by sizeTest after something is created.
//Lifetime allocations don't decrease ever.
//Alive allocations should increase by the size of the object created.
//After the object is deallocated, the alive allocations are expected to go back to the starting value.
inline void testExpectedAllocs( size_t lifetimeStart,
    size_t lifetimeAfterCreated,
    size_t lifetimeFinal,
    size_t aliveStart,
    size_t aliveAfterCreated,
    size_t aliveFinal,
    size_t sizeTest )
{
    //lifetime allocations should increase by the size of the allocated object
    EXPECT_NE(lifetimeStart, lifetimeFinal);
    EXPECT_EQ(lifetimeAfterCreated, lifetimeFinal); //Deleting the object doesn't substract from lifetime allocations.
    EXPECT_EQ(lifetimeFinal, lifetimeStart + sizeTest);
    //alive allocations should increase after creating the object, and then decrease to original value.
    EXPECT_NE(aliveStart, aliveAfterCreated);
    EXPECT_EQ(aliveAfterCreated, aliveStart + sizeTest);
    EXPECT_EQ(aliveFinal, aliveStart);

}
//Some random number to prevent lines of code getting rid of.
int getRandomNumber() {
    static std::random_device rd;
    static std::uniform_int_distribution<int> dist(0, 1000000);
    return dist(rd);
}
//Kind of hiding a variable so we can fool the compiler and avoid
//getting some lines of code removed due no-side-effects
double* externalVar() {
    static double v = 0;
    return &v;
}

TEST(TUBULNewDelete, NotUsingNew)
{
    const auto start = TU::memLifetime();
    //Doing simple stuff that DOESN'T USE THE HEAP, shouldn't affect the accounting.
    //This is just busywork to show we shouldn't touch the heap.
    auto res = 2+2;
    res += std::pow(res,3);
    if ( res > 50 )
        res += 2;
    else
        res +=1;
    //Some fibonacci for good measure;
    int fibs[20];
    fibs[0] = 0; fibs[1] = 1;
    for (size_t i = 2;i < 20; ++i )
        fibs[i] = fibs[i-1] + fibs[i-2];
    size_t acc = 0;
    for (size_t i = 0;i < 20; ++i )
        acc += fibs[i];


    const auto final = TU::memLifetime();
    EXPECT_EQ(start, final);
}


////////////////////////////////
///      WARNING             ///
////////////////////////////////
//These tests are way more convoluted than it should because the compiler
//plays against you!! If the compiler notices some pieces of code truly
//can't have side effects and do nothing real, they can be REMOVED. Meaning
//that the code you are trying to test can be REMOVED FROM THE TEST!!
//In our case, if the allocation of memory truly serves no purpose, it can
//be ignored!!! Meaning we won't create the objects and the memory won't be
//requested/deleted, making the tests fail. This has been discussed in the
//web as memory allocation is not clearly cut as not having side-effects, but
//the practical implication of skipping the allocation and saving time is winning the debate.



std::tuple<size_t, size_t> funcWithSingleStructure() {
    //The volatile here is crucial, if not this allocation can be ommited by optimizations!!
    SampleStruct* volatile pt = new SampleStruct;
    pt->a = getRandomNumber();
    pt->b = getRandomNumber();
    pt->c = std::sqrt(getRandomNumber());
    pt->d = '%';
    *externalVar() = (pt->a + pt->b + pt->c);
    auto ret = std::make_tuple(TU::memLifetime(), TU::memAlive());
    delete pt;
    return ret;
}



TEST(TUBULNewDelete, UsingNew)
{
    //On my tests it should be 24.
    constexpr auto sizeTest = sizeof(SampleStruct);
    const auto lifeStart = TU::memLifetime();
    const auto aliveStart = TU::memAlive();

    size_t lifeCreated = 0;
    size_t aliveCreated = 0;
    //Create a single structure. Should call our operator new
    std::tie( lifeCreated, aliveCreated) = funcWithSingleStructure();

    const auto lifeFinal= TU::memLifetime();
    const auto aliveFinal= TU::memAlive();

    testExpectedAllocs(lifeStart, lifeCreated, lifeFinal, aliveStart, aliveCreated, aliveFinal, sizeTest);

};

TEST(TUBULNewDelete, UsingNew2)
{
    //Because all the system of array allocation sucks, i need to
    //"prepare for the test" >:( just in case there hasn't been a previous
    //new[] call.
    {
        auto prep = new int[5];
        prep[0] = 1; prep[1] = 2;
        delete[] prep;
    }

    //On my tests the struct should be 24 and x20 is 480
    constexpr auto sizeTest = sizeof(SampleStruct);
    const auto lifeStart = TU::memLifetime();
    const auto aliveStart = TU::memAlive();

    size_t lifeCreated = 0;
    size_t aliveCreated = 0;
    static constexpr size_t ArraySize = 1024;
    //Create an array of structures.
    {
        SampleStruct volatile* pt = new SampleStruct[ArraySize] ;
        for (int i =0; i<ArraySize ; ++i) {
            pt[i].a = getRandomNumber(); pt[i].b = getRandomNumber(); pt[i].c = std::sqrt(getRandomNumber()); pt[i].d = '%';
            *externalVar() += (pt[i].a + pt[i].b + pt[i].c);
        }

        lifeCreated= TU::memLifetime();
        aliveCreated= TU::memAlive();
        delete[] pt;
        //Pointer should be released
    }
    const auto lifeFinal= TU::memLifetime();
    const auto aliveFinal= TU::memAlive();

    testExpectedAllocs(lifeStart, lifeCreated, lifeFinal, aliveStart, aliveCreated, aliveFinal, sizeTest*ArraySize);
}


std::tuple<size_t, size_t> funcWithVector() {
    //Create a single structure. Should call our operator new
    std::vector<SampleStruct> pt(20);
    auto lifeCreated = TU::memLifetime();
    auto aliveCreated = TU::memAlive();
    auto &ext = *externalVar();
    for (auto &i: pt) {
        i.a = getRandomNumber();
        i.b = getRandomNumber();
        i.c = sqrt(getRandomNumber());
        i.d = '%';
        ext += (i.a + i.b + i.c);
    }
    return {lifeCreated, aliveCreated};
}

TEST(TUBULNewDelete, UsingNewInVector)
{

    //On my tests it should be 24.
    constexpr auto sizeTest = sizeof(SampleStruct);
    const auto lifeStart = TU::memLifetime();
    const auto aliveStart = TU::memAlive();

    size_t lifeCreated = 0;
    size_t aliveCreated = 0;
    std::tie(lifeCreated, aliveCreated) = funcWithVector();

    const auto lifeFinal= TU::memLifetime();
    const auto aliveFinal= TU::memAlive();

    testExpectedAllocs(lifeStart, lifeCreated, lifeFinal, aliveStart, aliveCreated, aliveFinal, sizeTest*20);

}


////////////////////////////////
///  new[] / delete[] (#162) ///
////////////////////////////////

// Concurrent new[]/delete[] used to corrupt a global, unlocked registry and crash
// with SIGSEGV. After all threads join, alive must be exactly where it started.
TEST(TUBULNewDelete, ArrayNewDeleteFromManyThreads)
{
    constexpr int ThreadCount = 4;
    constexpr int Iterations = 20000;
    constexpr size_t ArrayBytes = 48;

    // Reserve first: the vector's own buffer is alive at assertion time and must not
    // be part of the delta being measured.
    std::vector<std::thread> threads;
    threads.reserve(ThreadCount);
    const auto aliveStart = TU::memAlive();
    const auto lifeStart = TU::memLifetime();

    for (int t = 0; t < ThreadCount; ++t)
        threads.emplace_back([] {
            for (int i = 0; i < Iterations; ++i) {
                // volatile: keeps the compiler from eliding the allocation.
                char* volatile a = new char[ArrayBytes];
                a[0] = 1;
                delete[] a;
            }
        });
    for (auto& th : threads)
        th.join();

    EXPECT_EQ(TU::memAlive(), aliveStart);
    // Lower bound: std::thread start-up also allocates, but never less than our arrays.
    EXPECT_GE(TU::memLifetime(), lifeStart + size_t(ThreadCount) * Iterations * ArrayBytes);
}

// Same, but each thread keeps a batch alive at once and frees it in a different
// order than it was allocated (the registry used to do swap-and-pop on this).
TEST(TUBULNewDelete, ArrayNewDeleteBatchesFromManyThreads)
{
    constexpr int ThreadCount = 4;
    constexpr int Rounds = 200;
    constexpr int Batch = 100;

    std::vector<std::thread> threads;
    threads.reserve(ThreadCount);
    const auto aliveStart = TU::memAlive();

    for (int t = 0; t < ThreadCount; ++t)
        threads.emplace_back([] {
            std::vector<char*> live(Batch);
            for (int r = 0; r < Rounds; ++r) {
                for (int i = 0; i < Batch; ++i) {
                    live[i] = new char[16 + i];
                    live[i][0] = char(i);
                }
                for (int i = 0; i < Batch; i += 2) delete[] live[i];
                for (int i = 1; i < Batch; i += 2) delete[] live[i];
            }
        });
    for (auto& th : threads)
        th.join();

    EXPECT_EQ(TU::memAlive(), aliveStart);
}

// delete[] used to do a linear search over every live array (quadratic overall).
// The bound is deliberately loose (the old code needed ~1.5 s here, the fix needs
// milliseconds) so it only trips on a real complexity regression, not on a slow CI box.
TEST(TUBULNewDelete, ArrayDeleteDoesNotScaleWithLiveArrays)
{
    constexpr size_t N = 100000;
    std::vector<char*> ptrs(N);
    // Measured after the vector's own (single) allocation so only the arrays count.
    const auto aliveStart = TU::memAlive();
    for (size_t i = 0; i < N; ++i) {
        ptrs[i] = new char[64];
        ptrs[i][0] = 1;
    }
    EXPECT_EQ(TU::memAlive(), aliveStart + N * 64);

    const auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < N; ++i)
        delete[] ptrs[i]; // FIFO, the slow order for the old linear search
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    EXPECT_LT(elapsed, 0.5);
    EXPECT_EQ(TU::memAlive(), aliveStart);
}

// Whatever bookkeeping new[] does, the pointer handed to the user must stay usable
// and suitably aligned, and the whole requested range must be writable.
TEST(TUBULNewDelete, ArrayNewAlignmentAndContents)
{
    for (size_t n : {size_t(1), size_t(3), size_t(8), size_t(17), size_t(1000), size_t(4096)}) {
        unsigned char* volatile p = new unsigned char[n];
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % __STDCPP_DEFAULT_NEW_ALIGNMENT__, 0u) << "n=" << n;
        for (size_t i = 0; i < n; ++i)
            p[i] = static_cast<unsigned char>(i * 7);
        for (size_t i = 0; i < n; ++i)
            ASSERT_EQ(p[i], static_cast<unsigned char>(i * 7)) << "n=" << n << " i=" << i;
        delete[] p;
    }
}

// Zero-length arrays are legal and must round-trip. new[0] is accounted as 1 byte
// (the override avoids malloc(0)), and delete[] must give that byte back.
TEST(TUBULNewDelete, ArrayNewZeroLength)
{
    const auto aliveStart = TU::memAlive();
    char* volatile p = new char[0];
    EXPECT_NE(p, nullptr);
    delete[] p;
    EXPECT_EQ(TU::memAlive(), aliveStart);
}

// Arrays of types with a non-trivial destructor get an array cookie and are released
// through the sized delete[]; make sure that path balances too.
namespace {
struct Counted {
    static inline int alive = 0;
    char pad[7];
    Counted() { ++alive; }
    ~Counted() { --alive; }
};
}

TEST(TUBULNewDelete, ArrayNewNonTrivialType)
{
    const auto aliveStart = TU::memAlive();
    Counted* volatile p = new Counted[10];
    EXPECT_EQ(Counted::alive, 10);
    delete[] p;
    EXPECT_EQ(Counted::alive, 0);
    EXPECT_EQ(TU::memAlive(), aliveStart);
}
