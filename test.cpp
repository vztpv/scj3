
// #include "vld.h"  // - check memory leak?


#include <iostream>


#include "mimalloc-new-delete.h"

#include "claujson.h"

#include "gtest/gtest.h"

// test value
    // test value init



int main(int argc, char* argv[])
{
    claujson::init(0); // must init! 

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
