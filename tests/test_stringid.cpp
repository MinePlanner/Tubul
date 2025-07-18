
#include <gtest/gtest.h>
#include <tubul.h>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <tuple>

// define some tags to create uniqueness
struct dog_tag{};
struct cat_tag{};

// create some type aliases for ease of use
CREATE_STRING_WRAPPER(DogId);
CREATE_STRING_WRAPPER(CatId);

TEST(TUBULStringId, basicComparison)
{
    DogId cachupin("Cachupin");
    CatId mrfluffles("MrFluffles");
    DogId other_dog = std::string("Ippo");

    EXPECT_EQ(cachupin.value(), DogId("Cachupin") );
    EXPECT_EQ(cachupin.value(), std::string("Cachupin") );
    EXPECT_TRUE(cachupin == "Cachupin" );//Explicit const char*
    EXPECT_TRUE(cachupin == std::string("Cachupin")); //Should auto-construct a dog name
    EXPECT_TRUE(cachupin == DogId("Cachupin")); //directly comparing dogIds
    EXPECT_NE(cachupin, other_dog);
    EXPECT_FALSE(cachupin == other_dog);
    EXPECT_TRUE(cachupin != other_dog);

    //Can't compare different types
    // EXPECT_NE(cachupin, mrfluffles); //DOESN'T COMPILE

    cachupin = DogId("Cachupin2.0");
    EXPECT_FALSE(cachupin == "Cachupin" );
    EXPECT_TRUE(cachupin == "Cachupin2.0" );

    //This method does work to "translate" one type to toher
    cachupin = mrfluffles.value();
    EXPECT_EQ( cachupin.value(), mrfluffles.value());

}

TEST(TUBULStringId, usingMap)
{
    //Creating names
    DogId cachupin("Cachupin");
    CatId mrfluffles("MrFluffles");
    DogId other_dog = std::string("Ippo");
    CatId other_cat = std::string("Mixie");


    //Can be used in a map, but you can't truly mix them
    std::map<DogId, CatId> dog_to_cat;

    dog_to_cat[cachupin] = mrfluffles;
    // dog_to_cat[mrfluffles] = mrfluffles; // DOESN'T COMPILE
    // dog_to_cat[mrfluffles] = cachupin;   // DOESN'T COMPILE
    // dog_to_cat[cachupin] = cachupin;     // DOESN'T COMPILE
    dog_to_cat[other_dog] = other_cat;


    std::vector< std::tuple<std::string, std::string> > expected =
            { {"Cachupin","MrFluffles"},
              {"Ippo","Mixie"} };
    size_t expit = 0;
    //We can iterate and it honors expected string ordering.
    for ( auto& [dog, cat]: dog_to_cat){
        EXPECT_EQ(std::tie(dog,cat), expected[expit]);
        ++expit;
    }
}
TEST(TUBULStringId, usingUnorderedMap)
{
    CatId foo("foo");  DogId bar("bar");

    //We can create an unordered map without any extra hassle
    std::unordered_map<CatId, DogId> cat_to_dog;
    cat_to_dog.emplace(foo, bar);
    cat_to_dog.emplace(CatId("foo2"), DogId("bar2"));

    //And works as expected. Same with easy comparisons
    EXPECT_EQ(cat_to_dog[CatId("foo")], DogId("bar"));
    EXPECT_EQ(cat_to_dog[foo], "bar");

    EXPECT_EQ(cat_to_dog[CatId("foo2")], "bar2");
}
