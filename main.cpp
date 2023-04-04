
// now, test only haswell..
// need C++17, 64bit..


#include "mimalloc-new-delete.h"


#include <iostream>
#include <string>
#include <ctime>

#include "claujson.h" // using simdjson 2.2.2

#include "_simdjson.h"

#include <cstring>


using namespace std::literals::string_view_literals;
// using namespace std::literals::u8string_view_literals; // ?? 

void utf_8_test() {
	using claujson::Value;

	// C++17 - stringview, C++20~ - u8string_view
	Value x(u8"こんにちは \\n wow hihi"sv); // no sv -> Data(bool)
	if (x) { // if before string is not valid utf-8, then x is not valid. x -> false
		auto& y = x.str_val();
		std::cout << y << "\n";
	}
}

void key_dup_test() {
	using claujson::Object;
	using claujson::Value;
	using claujson::Ptr;

	Ptr<Object> x = Ptr<Object>(new Object());

	x->add_object_element(Value("456"sv), Value(123));
	x->add_object_element(Value("123"sv), Value(234));
	x->add_object_element(Value("567"sv), Value(345));
	x->add_object_element(Value("456"sv), Value(456));

	size_t idx = 0;
	bool found = false;

	found = x->chk_key_dup(&idx);

	std::cout << found << " " << "idx is " << idx << "\n";
}

void json_pointer_test() {
	//	For example, given the JSON document

	auto test = u8R"({
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

	auto test2 = u8R"({
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
	using claujson::Value;

	Value x;
	if (!claujson::parse_str(test, x, 1).first) {
		std::cout << "fail\n";

		claujson::Ptr<claujson::Structured> clean(x.as_structured_ptr());

		return;
	}

	Value y;
	if (!claujson::parse_str(test2, y, 1).first) {
		std::cout << "fail\n";

		claujson::Ptr<claujson::Structured> clean(y.as_structured_ptr());

		return;
	}


	std::cout << x << "\n";

	{
		Value& y = x.json_pointer(""sv); // whole document...
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/foo"sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/foo/0"sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/"sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/a~1b"sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/c%d"sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/e^f"sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/g|h"sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer(R"(/i\\\\j)"sv); // chk R
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer(R"(/k\"l)"sv); // chk R
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/ "sv);
		std::cout << y << " ";
	}
	{
		Value& y = x.json_pointer("/m~0n"sv);
		std::cout << y << " ";
	}

	claujson::Value diff = claujson::diff(x, y);
	std::cout << diff << "\n";

	claujson::Value result = claujson::patch(x, diff);

	std::cout << result << "\n";

	if (result == y) {
		std::cout << "success\n";
	}

	{
		claujson::Ptr<claujson::Structured> clean(x.as_structured_ptr());
	}
	{
		claujson::Ptr<claujson::Structured> clean2(y.as_structured_ptr());
	}
	{
		claujson::Ptr<claujson::Structured> clean3(diff.as_structured_ptr());
	}
	{
		claujson::Ptr<claujson::Structured> clean4(result.as_structured_ptr());
	}
}

void str_test() {
	auto x = u8"한글 Test";

	claujson::Value A(x);

	if (!A.is_str()) {
		std::cout << "ERROR ";
	}

	auto y = "test";

	claujson::Value B(y);

	if (!B.is_str()) {
		std::cout << "ERROR2 ";
	}

	std::string z = "test";
	claujson::Value C(z);

	if (!C.is_str()) {
		std::cout << "ERROR3 ";
	}
	
}


int main(int argc, char* argv[])
{
	if (argc <= 1) {
		std::cout << "[program name] [json file name] \n";
		return 2;
	}

	/*
	std::cout << sizeof(claujson::Value) << "\n";
	std::cout << sizeof(claujson::Array) << "\n";
	std::cout << sizeof(claujson::Object) << "\n";
	{
		auto str = R"()"sv;

		claujson::init();

		claujson::Value ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto str = R"("A" : 3 )"sv;

		claujson::init();

		claujson::Value ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto str = R"(3,  3)"sv;

		claujson::init();

		claujson::Value ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{


		int a = std::chrono::steady_clock::now();
		_simdjson::dom::parser x;
		auto y = x.load("citylots.json");
		int b = std::chrono::steady_clock::now();
		std::cout << y.error() << " ";
		std::cout << b - a << "ms\n";

	}*/

	//claujson::log.no_print();

	//try 
	{
		claujson::init(0);
		claujson::log.console();

		utf_8_test();

		key_dup_test();

		json_pointer_test();

		str_test();

		for (int i = 0; i < 3; ++i) {
			claujson::Value j;
			bool ok;
			//try
			{

				auto a = std::chrono::steady_clock::now();

				// not-thread-safe..
				auto x = claujson::parse(argv[1], j, 0, true); // argv[1], j, 64 ??

				if (!x.first) {
					std::cout << "fail\n";

					claujson::clean(j);

					return 1;
				}

				auto b = std::chrono::steady_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
				std::cout << "total " << dur.count() << "ms\n";


			//	claujson::clean(j);

			//	return 0;

				//claujson::save("test12.txt", j);
				claujson::save_parallel("test34.json", j, 0);
				std::cout << "save_parallel" <<
					std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - b).count() << "ms\n";

				//claujson::clean(j);

			//return 0;

				//claujson::LoadData::save(std::cout, ut);
				//claujson::LoadData::save("output14.json", j);

				auto c = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);
				std::cout << "write " << dur.count() << "ms\n";

				int counter = 0;
				ok = x.first;

				std::vector<claujson::Value> vec;

				// json_pointer, json_pointerA <- u8string_view?

				if (false == claujson::Value::json_pointerA("/geometry/coordinates"sv, vec, false)) {
					std::cout << "json pointer error.\n";
					return 1;
				}

				double sum = 0;
				if (true && ok) {
					int chk = 0;
					for (int i = 0; i < 1; ++i) {
						if (j.is_structured()) {
							auto& features = j[1]; 
							for (auto& feature : features) { // feature["geometry"sv] <- no utf-8 str chk?, at("geometry"sv) : check valid utf-8 str?
								auto& coordinate = feature["geometry"sv]["coordinates"sv][0];  // feature.json_pointerB(vec)[0];  
								for (auto& coordinate_ : coordinate) {
									for (auto& x : coordinate_) {
										if (x.is_float()) {
											sum += x.float_val();
											counter++;
											chk++;
										}
									}
								}
							}
						}
					}
				}

				auto d = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(d - c);
				std::cout << dur.count() << "ms\n";
				std::cout << sum << " ";
				std::cout << counter << "  ";
				
				claujson::clean(j);
				return 0;

				auto c1 = std::chrono::steady_clock::now();

				//claujson::save("total_ends.json", j);

				// not thread-safe.
				claujson::save_parallel("total_end.json", j, 0);

				//claujson::save("total_ends.json", j);

				//std::cout << "\ncat \n";
				//system("cat total_end.json");
				//std::cout << "\n";

				auto c2 = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1);

				std::cout << "\nwrite " << dur.count() << "ms\n";

				claujson::Value X("geometry"sv); // in here, utf_8, unicode(\uxxxx) are checked..
				claujson::Value Y("coordinates"sv); // use claujson::Value.

				sum = 0; counter = 0;
				if (true && ok) {
					int chk = 0;
					for (int i = 0; i < 1; ++i) {
						auto& features = j.as_object()[1]; // j[1];
						for (auto& feature : features.as_array()) {
							auto& geometry = feature.as_object()[X.str_val()]; // as_array()[t].as_object()["geometry"];
							if (geometry.is_structured()) { // is_obj or arr?  -> is_structured
								auto& coordinates = geometry.as_object()[Y.str_val()]; // todo - add? at(const Data& data) ?
								auto& coordinate = coordinates.as_array()[0];
								for (auto& coordinate_ : coordinate.as_array()) {
									for (auto& x : coordinate_.as_array()) {
										if (x.is_float()) { // x.is_int(), x.is_uint() <- 
											sum += x.float_val();

											counter++;
											chk++;

											//claujson::Data test;
											//x = test; // no ok.
											//x = claujson::Data(0); // ok.
										}
									}
								}
							}
						}
					}
				}

				auto c3 = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(c3 - c2);

				std::cout << dur.count() << "ms\n";
				std::cout << "Re.. " << sum << " " << counter << "\n";

				claujson::clean(j);

				//std::cout << (claujson::error.has_error() ? ("has error") : ("no error")) << "\n";
				//std::cout << claujson::error.msg() << "\n";

				//
			}

			return !ok;
			/*catch (...) {
				if (j.is_ptr() && j.ptr_val()) {
					claujson::Ptr<claujson::Json> clean(&j.as_json());
				}

				std::cout << "internal error\n";
				return 1;
			}*/
		}

		{
			claujson::Value j;
			auto x = claujson::parse("total_end.json", j, 0); // argv[1], j, 64 ??
			if (!x.first) {
				std::cout << "fail\n";

				claujson::clean(j);

				return 1;
			}

			claujson::save_parallel("total_end2.json", j, 0);

			claujson::clean(j);
		}
	}
	//catch (...) {
	//	std::cout << "chk..\n";
		//return 1;
	//}
	//std::cout << (claujson::error.has_error() ? ( "has error" ) : ( "no error" )) << "\n";
	//std::cout << claujson::error.msg() << "\n";

	return 0;
}


