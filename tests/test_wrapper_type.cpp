#include <gtest/gtest.h>
#include <set>
#include "tubul.h"

TEST( TUBULWrapperType, TestWrapperInit)
{
    // We are going to create a wrapper for a size_t and ensure this
    // works as intended.
    CREATE_WRAPPER(WrapTestType, size_t);

    // ArcId should not be a simple size_t
    ASSERT_NE(typeid(size_t), typeid(WrapTestType));
    ASSERT_EQ(sizeof(size_t), sizeof(WrapTestType));
    // But it can be initialized from an int/size_t
    WrapTestType id1 { 42 };
    ASSERT_EQ(id1.value(), 42);

    // A size_t can't be initialized directly from a size_t wrapper,
    // but we can cast it.
    // size_t fromWrapper = id1;
    auto fromWrapper = (size_t)id1;

    ASSERT_EQ(fromWrapper, 42);
    ASSERT_EQ(fromWrapper, id1.value());

    auto fnWrapperParam = [](const WrapTestType& wrappedVal, size_t val) {
        ASSERT_EQ(wrappedVal.value(), val);
    };
    // We can pass a wrapped type to functions just normally.
    fnWrapperParam(id1, 42);
    // If the wrapper constructor is NOT EXPLICIT, then we can just easily pass a
    // size_t and a wrapped type is built automatically. This has pros and cons,
    // but for now (2025/02/13) it's preferred for simplicity as the main objective
    // of the wrappers is to not mix things like id's for different types of objects
    // which are size_t. If this behavior is to be changed, we can move the constructor
    // to be explicit
    // fnWrapperParam(42,42);
}

TEST(
    TUBULWrapperType, TestWrapperInSTL)
{

    CREATE_WRAPPER(WrapTestType, size_t);
    // We can store the wrapped types on vectors and use stuff like find/sort
    std::vector<WrapTestType> vec = { WrapTestType(5), WrapTestType(2), WrapTestType(4), WrapTestType(3), WrapTestType(1) };
    // We do have to compare with the wrapper type though.
    auto foundInVec = std::ranges::find(vec, WrapTestType(42));
    ASSERT_EQ(foundInVec, vec.end());
    foundInVec = std::ranges::find(vec, WrapTestType(1));
    ASSERT_EQ(foundInVec, vec.begin() + (vec.size() - 1)); // The one is at the last position.
    // After we reverse the vector, the now has to be at the first position
    std::reverse(vec.begin(), vec.end());
    ASSERT_EQ(vec.front().value(), 1);

    std::sort(vec.begin(), vec.end());
    for (size_t i = 0; i < vec.size(); i++)
        ASSERT_EQ(vec[i], WrapTestType(i + 1));

    // We can also use sets directly
    std::set<WrapTestType> set = { WrapTestType(5), WrapTestType(2), WrapTestType(4), WrapTestType(3), WrapTestType(1) };
    auto setIt = set.begin();
    for (size_t i = 0; setIt != set.end(); ++setIt, ++i)
        ASSERT_EQ(*setIt, WrapTestType(i + 1));

    // We can also use the wrapper types as if they were the underlying type
    // for hashing purposes
    std::unordered_map<WrapTestType, std::string> nameMap = {
        { WrapTestType(13), "nanu" },
        { WrapTestType(21), "nebula" },
        { WrapTestType(42), "rufus" },
        { WrapTestType(87), "drax" },
    };
    auto foundInMap = nameMap.find(WrapTestType(87));
    ASSERT_TRUE(foundInMap != nameMap.end() and foundInMap->second == "drax");
    foundInMap = nameMap.find(WrapTestType(7));
    ASSERT_TRUE(foundInMap == nameMap.end());
    foundInMap = nameMap.find(WrapTestType(13));
    ASSERT_TRUE(foundInMap != nameMap.end() and foundInMap->second == "nanu");
    // Because we can trivially build a wrapper type from the underlying type,
    // looking up in the map using size_t works. Not sure if we actually want that
    // but it's the current behavior and is reasonable considering the implementation.
    ASSERT_EQ(nameMap[WrapTestType(21)], "nebula");
    ASSERT_EQ(nameMap[WrapTestType(42)], "rufus");

    WrapTestType id1 { 42 };
    CREATE_WRAPPER(WrapTestTypeOther, size_t);
    ASSERT_EQ(sizeof(size_t), sizeof(WrapTestTypeOther));
    ASSERT_NE(typeid(WrapTestType), typeid(WrapTestTypeOther));
    WrapTestTypeOther id2 { 42 };
    ASSERT_EQ(id1.value(), 42);
    ASSERT_EQ(id2.value(), 42);
    ASSERT_EQ(id2.value(), id1.value());
    id2 = WrapTestTypeOther(54);
    ASSERT_NE(id2.value(), id1.value());
    // This should not compile because the type system will reject these
    // wrappers as being the same type
    // ASSERT_NE(id1, id2);
}

