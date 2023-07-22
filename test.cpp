
// #include "vld.h"  // - check memory leak?


#include <iostream>


#include "mimalloc-new-delete.h"

#include "claujson.h"

#include "gtest/gtest.h"

// test value
    // test value init
TEST(ValueTest, ValueInitTest_int)
{
    claujson::Value j(1);
    EXPECT_TRUE(j);
    EXPECT_EQ(j.is_int(), true); // is_integer?
    EXPECT_EQ(j.get_integer(), 1); // get_int?
    claujson::clean(j);
}
TEST(ValueTest, ValueInitTest_double)
{
    claujson::Value j(1.5);
    EXPECT_TRUE(j);
    EXPECT_EQ(j.is_float(), true); // is_floating?
    EXPECT_EQ(j.get_floating(), 1.5); // get_float?
    claujson::clean(j);
}
TEST(ValueTest, ValueInitTest_string)
{
    claujson::Value j("test");
    EXPECT_TRUE(j);
    EXPECT_EQ(j.is_str(), true); // is_string?
    EXPECT_EQ(j.get_string(), "test");
    claujson::clean(j);
}
TEST(ValueTest, ValueInitTest_bool)
{
    claujson::Value j(true);
    EXPECT_TRUE(j);
    EXPECT_EQ(j.is_bool(), true); // is_boolean?
    EXPECT_EQ(j.get_boolean(), true); // get_bool?
    claujson::clean(j);
}
TEST(ValueTest, ValueInitTest_nullptr) // cf) NULL ?
{
    claujson::Value j(nullptr);
    EXPECT_TRUE(j);
    EXPECT_EQ(j.is_null(), true);
    claujson::clean(j); 
}
  
    // test Value.as~
TEST(ValueTest, ValueAsTest1)
{
    claujson::Value j("test");
    EXPECT_TRUE(j);
    EXPECT_EQ(j.as_array().is_valid(), false);
    EXPECT_EQ(j.as_object().is_valid(), false);
    EXPECT_EQ(j.as_structured_ptr(), nullptr);
    claujson::clean(j);
}

TEST(ValueTest, ValueAsTest2)
{
    claujson::Value j(new claujson::Array());
    EXPECT_TRUE(j);
    EXPECT_TRUE(j.as_array().is_valid());
    EXPECT_EQ(j.as_object().is_valid(), false);
    EXPECT_TRUE(j.as_structured_ptr() != nullptr);
    claujson::clean(j);
}

TEST(ValueTest, ValueAsTest3)
{
    claujson::Value j(new claujson::Object());
    EXPECT_FALSE(j.as_array().is_valid());
    EXPECT_TRUE(j.as_object().is_valid());
    EXPECT_TRUE(j.as_structured_ptr() != nullptr);
    claujson::clean(j); 
}
    // test begin, end
TEST(ValueTest, ValueBeginEndTest1)
{
    claujson::Value j(12345);

    EXPECT_NO_THROW(
        {
            for (auto& x : j) {
                throw 2;
            }
        }
    );

    claujson::clean(j);
}
TEST(ValueTest, ValueBeginEndTest2)
{
    claujson::Value j(new claujson::Array());
    for (size_t i = 0; i < 1024; ++i) {
        j.as_array().add_array_element(claujson::Value(i));
    }

    EXPECT_NO_THROW(
        {
            int a = 0;
            for (auto& x : j) {
                if (x.get_integer() != a) {
                    throw 1;
                }
                ++a;
            }
        }
    );

    claujson::clean(j);
}
    // test json pointer - need more test...!

// test array
TEST(ArrayTest, ArrayInitTest)
{
    claujson::Value j(claujson::Array::Make());
    EXPECT_TRUE(j);
    EXPECT_TRUE(j.is_array());
    EXPECT_TRUE(j.as_array().is_valid());
    EXPECT_TRUE(j.as_array().get_data_size() == 0);
    EXPECT_TRUE(j.as_array().size() == 0); //
    EXPECT_TRUE(j.as_array().empty()); // 
    claujson::clean(j);
}

TEST(ArrayTest, ArrayAddElementTest)
{
    // Array::Make() <- [[nodiscard]] ?
    claujson::Value j(claujson::Array::Make()); // todo - remove Array public constructor? only use claujson::Array::Make() ?
    claujson::Array* arr = dynamic_cast<claujson::Array*>(j.as_structured_ptr());
    EXPECT_TRUE(arr != nullptr);
    EXPECT_TRUE(arr->add_array_element(claujson::Value(1)));
    EXPECT_FALSE(arr->add_object_element(claujson::Value("key"), claujson::Value(123))); // is not object
    EXPECT_FALSE(arr->add_object_element(claujson::Value(true), claujson::Value(123))); // is not object
    EXPECT_FALSE(arr->change_key(claujson::Value("Test"), claujson::Value("Test2")));
    claujson::clean(j);
}

// test object
TEST(ObjectTest, ObjectInitTest)
{
    claujson::Value j(claujson::Object::Make()); // Make returns claujson::Object* 
    EXPECT_TRUE(j); // convert to j.is_valid() 
    EXPECT_TRUE(j.is_object());
    EXPECT_TRUE(j.as_object().is_valid());
    EXPECT_TRUE(j.as_object().get_data_size() == 0);
    EXPECT_TRUE(j.as_object().size() == 0); // added
    EXPECT_TRUE(j.as_object().empty()); // added
    claujson::clean(j);
}

TEST(ObjectTest, ObjectAddElementTest)
{
    claujson::Value j(claujson::Object::Make()); // Make returns claujson::Object*, claujson::Value no delete child...?
    claujson::Object* obj = dynamic_cast<claujson::Object*>(j.as_structured_ptr());
    EXPECT_TRUE(obj != nullptr);
    EXPECT_FALSE(obj->add_array_element(claujson::Value(1))); // is not array.
    EXPECT_TRUE(obj->add_object_element(claujson::Value("key"), claujson::Value(123))); // claujson::Value is long? claujson->cj? cj::Value("key") ?
    EXPECT_FALSE(obj->add_object_element(claujson::Value(true), claujson::Value(123))); // first is not key.
    claujson::clean(j);
}

// remove method?
TEST(ObjectTest, ObjectAddValueTest) // here, Value is array or object.
{
    claujson::Value j(claujson::Object::Make());
    //j.as_object().add_array() // code is too long? 
    //j.as_object().add_object() // claujson::Ptr<Structured> -> claujson::Value ? 
    claujson::clean(j);
}

// test partial_json - not used by user. - how test? - friend keyword????

// tests from scj/main.cpp ?

// test iterator.

// test parse json
    // cf) simdjson

// test save json
    // save_to_string??
    // save_to_file?
// using test with simdjson? or
//   save vs save_with_parallel ?    


// test parser. // check std::thread number, more variable?
TEST(ParserTest, EmptyArray) 
{
    claujson::Value j; 
                                                 // use more fast if big file?
    EXPECT_EQ(true, claujson::parse_str("[]", j, 1, false).first);

    claujson::clean(j);
    EXPECT_EQ(true, claujson::parse_str("[]", j, 1, true).first);

    claujson::clean(j);
    EXPECT_EQ(true, claujson::parse_str("[]", j, 0, false).first);

    claujson::clean(j);
    EXPECT_EQ(true, claujson::parse_str("[]", j, 0, true).first);

    claujson::clean(j);
}

TEST(ParserTest, EmptyObject)
{
    claujson::Value j;

    EXPECT_EQ(true, claujson::parse_str("{}", j, 1, false).first);

    claujson::clean(j);
    EXPECT_EQ(true, claujson::parse_str("[]", j, 1, true).first);

    claujson::clean(j);
    EXPECT_EQ(true, claujson::parse_str("[]", j, 0, false).first);

    claujson::clean(j);
    EXPECT_EQ(true, claujson::parse_str("[]", j, 0, true).first);
    
    claujson::clean(j);
}

// test valid json.
    // test json <- one element
    
// test wrong json. - from simdjson test? - this case.... check license..
    // test wrong utf8
    // test wrong unicode in " ~~ "
    // test wrong json grammar
    // test wrong string.
    // test wrong number.


// test json-string`s substr().
    // merge // result maybe has virtual array or virtual object.
    // 


int main(int argc, char* argv[])
{
    claujson::init(0); // must init! 

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
