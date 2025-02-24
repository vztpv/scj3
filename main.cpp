
// now, test only haswell..
// need C++14~, 64bit..
// mainly tested with C++17...

#include "mimalloc-new-delete.h"

#include <iostream>
#include <string>
#include <ctime>


//#define USE_PMR // with c++17~

#include "claujson.h" // using simdjson 3.9.1

#include "_simdjson.h"

#include <cstring>

// using namespace std::literals::u8string_view_literals; // ?? 

void utf_8_test() {
	using claujson::_Value;

	// C++17 - stringview, C++20~ - u8string_view
	_Value x(u8"こんにちは \\n wow hihi"sv); // no sv -> Data(bool)
	if (x) { // if before string is not valid utf-8, then x is not valid. x -> false
		auto& y = x.str_val();
		std::cout << y.data() << "\n";
	}
}

void key_dup_test() {
	using claujson::Object;
	using claujson::_Value;
	using claujson::Ptr;

	Ptr<Object> x = Ptr<Object>(new Object());

	x->add_element(_Value("456"sv), _Value(123));
	x->add_element(_Value("123"sv), _Value(234));
	x->add_element(_Value("567"sv), _Value(345));
	x->add_element(_Value("456"sv), _Value(456));

	uint64_t idx = 0;
	bool found = false;

	found = x->chk_key_dup(&idx);

	std::cout << found << " " << "idx is " << idx << "\n";
}

void str_test() {
	auto x = u8"한글 Test";

	claujson::_Value A(x);

	if (!A.is_str()) {
		std::cout << "ERROR ";
	}

	auto y = "test";

	claujson::_Value B(y);

	if (!B.is_str()) {
		std::cout << "ERROR2 ";
	}

	std::string z = "test";
	claujson::_Value C(z);

	if (!C.is_str()) {
		std::cout << "ERROR3 ";
	}
	
}

void diff_test() {
	std::cout << "diff test\n";

	std::string json1 = "{ \"abc\" : [ 1,2,3] }";
	std::string json2 = "{ \"abc\" : [ 2,4,5] }";
	
	claujson::Document x, y;
	claujson::parser p;
	p.parse_str(json1, x, 0);
	p.parse_str(json2, y, 0);

	std::cout << x.Get() << "\n";
	claujson::Document z = claujson::diff(x.Get(), y.Get());

	std::cout << z.Get() << "\n";

	auto& k = claujson::patch(x.Get(), z.Get()); // chk!
	std::cout << k << "\n";

	//claujson::clean(x);
	//claujson::clean(y);
	//claujson::clean(z);
}

/*
// iterator test.
#include <functional>
namespace claujson {
	class JsonIterator {
	public:
		JsonIterator& enter(std::function<void(Structured*)> func = { }) {
			if (_m_now && _m_now->get_value_list(_m_child_pos_in_parent.back()).is_structured()) {
				_m_now = _m_now->get_value_list(_m_child_pos_in_parent.back()).as_structured_ptr();
				_m_child_pos_in_parent.push_back(0);
				if (func) {
					func(_m_now);
				}
			}
			return *this;
		}
		
		JsonIterator& quit(std::function<void(Structured*)> func = { }) {
			if (_m_now) {
				_m_child_pos_in_parent.pop_back();
				_m_now = _m_now->get_parent();
				if (func) {
					func(_m_now);
				}
			}
			return *this;
		}

		JsonIterator& next(std::function<void(Structured*)> func = { }) {
			if (_m_now) {
				_m_child_pos_in_parent.back()++;
				if (func) {
					func(_m_now);
				}
			}
			return *this;
		}

		JsonIterator& iterate(std::function<void(_Value&)> func) {
			if (_m_now) {
				size_t len = _m_now->get_data_size();
				for (size_t i = _m_child_pos_in_parent.back(); i < len; ++i) {
					func(_m_now->get_value_list(i));
				}
			}
			return *this;
		}

		bool is_valid() const {
			return _m_now && _m_child_pos_in_parent.empty() == false && _m_child_pos_in_parent.back() < _m_now->get_data_size();
		}

		_Value& now() {
			if (_m_now) {
				return _m_now->get_value_list(_m_child_pos_in_parent.back());
			}
			static _Value null_data(nullptr, false);
			return null_data;
		}

	public:
		JsonIterator(Structured* arr_or_obj) : _m_now(arr_or_obj) {
			_m_child_pos_in_parent.push_back(0);
		}
	private:
		Structured* _m_now = nullptr;
		std::vector<size_t> _m_child_pos_in_parent;
	};
}
*/

int main(int argc, char* argv[])
{
	std::cout << sizeof(std::vector<std::pair<claujson::_Value, claujson::_Value>>) << "\n";
	//std::cout << sizeof(std::string) << " " << sizeof(claujson::Structured) << " " << sizeof(claujson::Array)
	//	<< " " << sizeof(claujson::Object) << " " << sizeof(claujson::_Value) << "\n";

	if (argc <= 1) {
		std::cout << "[program name] [json file name] (number of thread) \n";
		return 2;
	}

	diff_test();

	/*
	std::cout << sizeof(claujson::_Value) << "\n";
	std::cout << sizeof(claujson::Array) << "\n";
	std::cout << sizeof(claujson::Object) << "\n";
	{
		auto str = R"()"sv;

		claujson::init(0);

		claujson::_Value ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto str = R"("A" : 3 )"sv;

		claujson::init(0);

		claujson::_Value ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto str = R"(3,  3)"sv;

		claujson::init(0);

		claujson::_Value ut;
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
		//claujson::init(24);



		// log test.
		//claujson::log.no_print();
		//claujson::log.console();
		//claujson::log.info();
		//claujson::log.warn();
		//claujson::log.info(true);
		//claujson::log.warn(true);

	//	utf_8_test();

	//	key_dup_test();

	//	json_pointer_test();

	//	str_test();


		{
			claujson::StringView s{ "abc", 3 };
			claujson::StringView x{ "abcg", 4 };

			std::cout << s.compare(x) << "\n";
			std::cout << x.compare(s) << "\n";
		}

				int thr_num = 0;

	if (argc > 2) {
					thr_num = std::atoi(argv[2]);
				}

		claujson::parser p(thr_num);

		for (int i = 0; i < 1; ++i) {
			claujson::Document j;
			
			if (argc < 4) {
				claujson::log.console();
				claujson::log.info(); // info도 보임
				claujson::log.warn(); // warn도 보임
			}
			bool ok;
			//try
			{	
				if (0) {
					auto a = std::chrono::steady_clock::now();
					
					static	_simdjson::dom::parser test;

						auto x = test.load(argv[1]);
					
					auto b = std::chrono::steady_clock::now();
					auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
					std::cout << "simdjson " << dur.count() << "ms\n";
				}


				auto a = std::chrono::steady_clock::now();


			

				// not-thread-safe..
				auto x = p.parse(argv[1], j, thr_num); // argv[1], j, 64 ??
				//claujson::Document d = std::move(j);

				if (!x.first) {
					std::cout << "fail\n";

					return 1;
				}
				

			// return 0;

				auto b = std::chrono::steady_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
				std::cout << "total " << dur.count() << "ms\n";
			//	continue;
				auto c = std::chrono::steady_clock::now();
				{
					auto z = j.Get().clone();
					c = std::chrono::steady_clock::now();
					claujson::clean(z);
				}

				
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);
				std::cout << "total " << dur.count() << "ms\n";

				//return 0;

			//	continue;
				//debug test
			//	//std::cout << j << "\n";
			//	std::cout << "chk\n";
			//	return 0;
				//
				// 
				
				claujson::writer w;

				w.write_parallel("temp.json", j.Get(), 0, true);
				
				std::cout << "write_parallel " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - c).count() << "ms\n";
				
				if (1) {

					claujson::Document x;
					auto result = p.parse("temp.json", x, thr_num);

					if (!result.first) {
						return 1;
					}

					//
					//claujson::Document _diff = claujson::diff(j.Get(), x.Get());

					//if (_diff.Get().is_valid() && _diff.Get().as_structured_ptr() && _diff.Get().as_array()->empty() == false) {
					//	std::cout << "diff \n";//return 1;
					//}
				}

				//claujson::clean(j);
				//return 0;
				//b = std::chrono::steady_clock::now();
				//test::write("test78.json", j);

			//	std::cout << "write " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - b).count() << "ms\n";


				//claujson::clean(j);

				//return 0;

				//claujson::LoadData::write(std::cout, ut);
				//claujson::LoadData::write("output14.json", j);
//
				c = std::chrono::steady_clock::now();
				//dur = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);
				//std::cout << "write " << dur.count() << "ms\n";

				int counter = 0;
				ok = x.first;

				std::vector<claujson::_Value> vec;

				// json_pointer, json_pointerA <- u8string_view?

				static const auto _geometry = claujson::_Value("geometry"sv);
				static const auto _coordinates = claujson::_Value("coordinates"sv);

				double sum = 0;
				if (true && ok) {
					for (int i = 0; i < 1; ++i) {
						if (j.Get().is_structured()) {
							auto& features = j.Get()[1];
							claujson::Array* features_arr = features.as_array(); // as_array_ptr() ?
							if (!features_arr) {
								continue;
							}
							for (auto& feature : *features_arr) { // feature["geometry"sv] <- no utf-8 str chk?, at("geometry"sv) : check valid utf-8 str?
								auto& coordinate = feature[_geometry][_coordinates][0];  // feature.json_pointerB(vec)[0];  
								claujson::Array* coordinate_arr = coordinate.as_array();
								if (!coordinate_arr) {
									continue;
								}
								for (auto& coordinate_ : *coordinate_arr) {
									claujson::Array* coordinate__arr = coordinate_.as_array();
									if (!coordinate__arr) {
										continue;
									}
									for (auto& x : *coordinate__arr) {
										if (x.is_float()) {
											sum += x.float_val();
											x.set_int(x.float_val());
											//x.set_str(u8"te한st", 7);
											counter++;
										}
									}
								}
							}
						}
					}
				}

				auto dd = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(dd - c);
				std::cout << dur.count() << "ms\n";
				std::cout << sum << " ";
				std::cout << counter << "  ";
				
				c = std::chrono::steady_clock::now();
				//claujson::clean(j);
				dd = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(dd - c);
				std::cout << "clean " <<  dur.count() << "ms\n";

				w.write_parallel("test2333.json", j.Get(), 0, true);
				return 0;

				
			//	claujson::clean(j);
				return 0;

				auto c1 = std::chrono::steady_clock::now();

				//claujson::write("total_ends.json", j);

				// not thread-safe.
				w.write_parallel("total_end.json", j.Get(), 0);

				//claujson::write("total_ends.json", j);

				//std::cout << "\ncat \n";
				//system("cat total_end.json");
				//std::cout << "\n";

				auto c2 = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1);

				std::cout << "\nwrite " << dur.count() << "ms\n";

				claujson::_Value X("geometry"sv); // in here, utf_8, unicode(\uxxxx) are checked..
				claujson::_Value Y("coordinates"sv); // use claujson::_Value.

				sum = 0; counter = 0;
				/*if (true && ok) {
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
				}*/

				auto c3 = std::chrono::steady_clock::now();
				dur = std::chrono::duration_cast<std::chrono::milliseconds>(c3 - c2);

				std::cout << dur.count() << "ms\n";
				std::cout << "Re.. " << sum << " " << counter << "\n";

			//	claujson::clean(j);

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
			claujson::parser p;
			claujson::Document j;
			auto x = p.parse("total_end.json", j, 0); // argv[1], j, 64 ??
			if (!x.first) {
				std::cout << "fail\n";

				//claujson::clean(j);

				return 1;
			}
			claujson::writer w;
			w.write_parallel("total_end2.json", j.Get(), 0);

			//claujson::clean(j);
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

/* result.
simdjson 236ms

[INFO] simdjson - stage1 start
[INFO] 83ms
[INFO] valid1 0ms
[INFO] test time 7ms
[INFO] 7ms
[INFO] parse1 42ms
[INFO] test6[INFO] parse2 4ms
[INFO] chk 0ms
[INFO] 46ms
[INFO] 138ms
total 139ms */