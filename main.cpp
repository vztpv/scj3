
// now, test only haswell..
// need C++17, 64bit..

#include "mimalloc-new-delete.h"

#include <iostream>
#include <string>
#include <ctime>

#include "claujson.h" // using simdjson 2.2.2

#include "simdjson.h"

#include <cstring>


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
		
		int a = clock();
		_simdjson::dom::parser x;
		auto y = x.load("citylots.json");
		int b = clock();
		std::cout << y.error() << " ";
		std::cout << b - a << "ms\n";
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

				int a = clock();

				// not-thread-safe..
				auto x = claujson::parse(argv[1], j, 64); // argv[1], j, 64 ??

				if (!x.first) {
					std::cout << "fail\n";

					claujson::clean(j);

					return 1;
				}

				int b = clock();
				std::cout << "total " << b - a << "ms\n";

				claujson::clean(j);
				return 0;

				//claujson::LoadData::save(std::cout, ut);
				//claujson::LoadData::save("output14.json", j);

				int c = clock();
				std::cout << "write " << c - b << "ms\n";

				int counter = 0;
				ok = x.first;

				std::vector<claujson::Data> vec;

				// json_pointer, json_pointerA <- u8string_view?

				if (false == claujson::Data::json_pointerA("/geometry/coordinates"sv, vec)) {
					std::cout << "json pointer error.\n";
					return 1;
				}

				double sum = 0;
				if (true && ok) {
					int chk = 0;
					for (int i = 0; i < 1; ++i) {
						if (j.is_structured()) {
							auto& features = j.as_object()[1]; // j[1];
							for (auto& feature : features.as_array()) {
								auto& coordinate = feature.json_pointerB(vec).as_array()[0];  // { vec, op } // <- class??
								for (auto& coordinate_ : coordinate.as_array()) {
									for (auto& x : coordinate_.as_array()) {
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


				std::cout << clock() - c << "ms\n";
				std::cout << sum << " ";
				std::cout << counter << "  ";
				//return 0;

				int c1 = clock();

				//claujson::save("total_ends.json", j);

				// not thread-safe.
				claujson::save_parallel("total_end.json", j, 64);

				//claujson::save("total_ends.json", j);

				//std::cout << "\ncat \n";
				//system("cat total_end.json");
				//std::cout << "\n";

				int c2 = clock();
				std::cout << "\nwrite " << c2 - c1 << "ms\n";

				claujson::Data X("geometry"sv); // in here, utf_8, unicode(\uxxxx) are checked..
				claujson::Data Y("coordinates"sv); // use claujson::Data.

				sum = 0; counter = 0;
				if (true && ok) {
					int chk = 0;
					for (int i = 0; i < 1; ++i) {
						auto& features = j.as_object()[1]; // j[1];
						for (auto& feature : features.as_array()) {
							auto& geometry = feature.as_object().at(X.str_val()); // as_array()[t].as_object()["geometry"];
							if (geometry.is_structured()) { // is_obj or arr?  -> is_structured
								auto& coordinates = geometry.as_object().at(Y.str_val()); // todo - add? at(const Data& data) ?
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
				std::cout << clock() - c2 << "ms\n";
				std::cout << "Re.. " << sum << " " << counter << "\n";

				claujson::Ptr<claujson::Json> clean(j.as_json_ptr());

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
			claujson::Data j;
			auto x = claujson::parse("total_end.json", j, 64); // argv[1], j, 64 ??
			if (!x.first) {
				std::cout << "fail\n";

				claujson::clean(j);

				return 1;
			}

			claujson::save_parallel("total_end2.json", j, 64);

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


