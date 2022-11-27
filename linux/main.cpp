
// now, test only haswell..
// need C++17, 64bit..

#include "mimalloc-new-delete.h"

#include <iostream>
#include <string>
#include <ctime>

#include "claujson.h" // using simdjson 2.2.2

#include "simdjson.h"

#include <cstring>
#include <chrono>

using namespace std::literals::string_view_literals;
// using namespace std::literals::u8string_view_literals; // ?? 

void utf_8_test() {
	using claujson::Data;

	// C++17 - stringview, C++20~ - u8string_view
	Data x(u8"こんにちは \\n wow hihi"sv); // no sv -> Data(bool)
	if (x) {
		auto& y = x.str_val();
		std::cout << y << "\n";
	}
}

void key_dup_test() {
	using claujson::Object;
	using claujson::Data;
	using claujson::Ptr;

	Ptr<Object> x = Ptr<Object>(new Object());

	x->add_object_element(Data("456"sv), Data(123));
	x->add_object_element(Data("123"sv), Data(234));
	x->add_object_element(Data("567"sv), Data(345));
	x->add_object_element(Data("456"sv), Data(456));

	size_t idx = 0;
	bool found = false;

	found = x->chk_key_dup(&idx);

	std::cout << found << " " << "idx is " <<  idx << "\n";
}

void json_pointer_test() {
	//	For example, given the JSON document

	std::string test = u8R"({
		   "foo": ["bar", "baz"] ,
		   "" : 0,
		   "a/b" : 1,
		   "c%d" : 2,
		   "e^f" : 3,
		   "g|h" : 4,
		   "i\\\\j" : 5,
		   "k\"l" : 6,
		   " " : 7,
		   "m~n" : 8
		})";

	std::string test2 = u8R"({
		   "foo": ["bar2", "baz"] ,
		   "" : 0,
		  
		   "c%d" : 2,
		   "e^f" : 3,
		   "g|h" : 45,
		   "i\\\\j" : 5,
		   "k\"l" : 6,
		   " " : 7,
		   "m~n" : 8,
		   "zzz": 9
		})";

	//	The following JSON strings evaluate to the accompanying values :

//	""           // the whole document
//		"/foo"    ["bar", "baz"]
//		"/foo/0"     "bar"
//		"/"          0
//		"/a~1b"      1
//		"/c%d"       2
//		"/e^f"       3
//		"/g|h"       4
//		"/i\\j"      5
//		"/k\"l"      6
//		"/ "         7
//		"/m~0n"      8
	using claujson::Data;

	Data x;
	if (!claujson::parse_str(test, x, 1).first) {
		std::cout << "fail\n";

		claujson::Ptr<claujson::Json> clean(x.as_json_ptr());

		return;
	}

	Data y;
	if (!claujson::parse_str(test2, y, 1).first) {
		std::cout << "fail\n";

		claujson::Ptr<claujson::Json> clean(y.as_json_ptr());

		return;
	}


	std::cout << x << "\n";

	{
		Data& y = x.json_pointer(""sv); // whole document...
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/foo"sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/foo/0"sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/"sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/a~1b"sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/c%d"sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/e^f"sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/g|h"sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer(R"(/i\\\\j)"sv); // chk R
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer(R"(/k\"l)"sv); // chk R
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/ "sv);
		std::cout << y << " ";
	}
	{
		Data& y = x.json_pointer("/m~0n"sv);
		std::cout << y << " ";
	}

	claujson::Data diff = claujson::diff(x, y);
	std::cout << diff << "\n";

	claujson::Data result = claujson::patch(x, diff);

	std::cout << result << "\n";

	if (result == y) {
		std::cout << "success\n";
	}

	{
		claujson::Ptr<claujson::Json> clean(x.as_json_ptr());
	}
	{
		claujson::Ptr<claujson::Json> clean2(y.as_json_ptr());
	}
	{
		claujson::Ptr<claujson::Json> clean3(diff.as_json_ptr());
	}
	{
		claujson::Ptr<claujson::Json> clean4(result.as_json_ptr());
	}
}


int main(int argc, char* argv[])
{

	{
		auto str = R"("A" : 3 )"sv;

		claujson::init();

		claujson::Data ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto str = R"(3,  3)"sv;

		claujson::init();

		claujson::Data ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto a = std::chrono::steady_clock::now();

		_simdjson::dom::parser x;
		auto y = x.load(argv[1]);

		auto b = std::chrono::steady_clock::now();
		std::cout << y.error() << " ";
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

		std::cout << "pre " << dur.count() << "ms\n";
	}

	//claujson::log.no_print();

	//try 
	{
		claujson::init();
		claujson::log.console();
		//utf_8_test();

		//key_dup_test();

		//json_pointer_test();


		for (int i = 0; i < 3; ++i) {
			claujson::Data j;
			bool ok;
			//try
			{

				auto a = std::chrono::steady_clock::now();

				// not-thread-safe..
				auto x = claujson::parse(argv[1], j, 64); // argv[1], j, 64 ??

				if (!x.first) {
					std::cout << "fail\n";

					claujson::clean(j);

					return 1;
				}

				auto b = std::chrono::steady_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

				std::cout << "total " << dur.count() << "ms\n";

				claujson::clean(j);
			}
			{


				auto a = std::chrono::steady_clock::now();

				// not-thread-safe..
				auto x = claujson::parse(argv[1], j, 1); // argv[1], j, 64 ??

				if (!x.first) {
					std::cout << "fail\n";

					claujson::clean(j);

					return 1;
				}

				auto b = std::chrono::steady_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

				std::cout << "total " << dur.count() << "ms\n";

				claujson::clean(j);
			}

			return 0;
		}
	}
}



