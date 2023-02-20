
#include <iostream>

#include "claujson.h"

#include "gtest/gtest.h"

TEST(ParserTest, EmptyArray)
{
    claujson::Value j;

    EXPECT_EQ(true, claujson::parse_str("[]", j, 1, false).first);
}


int main(int argc, char* argv[]) 
{
    claujson::init(0); // must!

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
