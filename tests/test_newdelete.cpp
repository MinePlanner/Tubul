//
// Created by Carlos Acosta on 15-02-24.
//
#include <gtest/gtest.h>
#include <random>
#include <cmath>
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
