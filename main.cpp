
// now, test only haswell..
// need C++14~, 64bit..
// mainly tested with C++17...

//#define _CRTDBG_MAP_ALLOC
//#include <cstdlib>
//#include <crtdbg.h>

#include "mimalloc-new-delete.h"

#include <iostream>
#include <string>
#include <ctime>

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

	//int* yy = new int[100];
	//claujson::clean(x);
	//claujson::clean(y);
	//claujson::clean(z);
}

/*
enum class ValueType {
	none,
	end_of_container,
	end_of_document,
	container, // array or object
	item,
	key,
	value
};

class ClauJsonTraverser {
	// todo!
};

class DiffResult {
public:
	ClauJsonTraverser x;
	ClauJsonTraverser y;
	claujson::_Value data; // key or value.
	int line = 0;
	int type = 0; // -1 delete, +1 add
	ValueType x_type;
	ValueType y_type;
public:
	DiffResult(int line, const ClauJsonTraverser& x, ValueType x_type, const ClauJsonTraverser& y,
		ValueType y_type,
		int type, claujson::_Value&& data)
		: line(line), x(x), x_type(x_type), y(y), y_type(y_type), type(type), data(std::move(data))
	{
		//
	}
};

std::vector<DiffResult> Diff(claujson::StructuredPtr before, claujson::StructuredPtr after) {
	std::vector<DiffResult> result;

	if (before == nullptr || after == nullptr) {
		return result;
	}

	std::map<claujson::_ValueView, size_t> before_map;
	std::map<claujson::_ValueView, size_t> after_map;

	ClauJsonTraverser iter_before(before);
	ClauJsonTraverser iter_after(after);

	ClauJsonTraverser x = iter_before;
	ClauJsonTraverser y = iter_after;

	ClauJsonTraverser __x = x;
	ClauJsonTraverser __y = y;

	ClauJsonTraverser temp_x = x;
	ClauJsonTraverser temp_y = y;

	ClauJsonTraverser temp_ = y;

	ClauJsonTraverser before_x = x;
	ClauJsonTraverser before_y = y;

	claujson::_ValueView temp;
	claujson::_ValueView str1;
	claujson::_ValueView str2;

	claujson::_ValueView same_value;

	int line = 0;

	int state = 0;

	{
		ClauJsonTraverser test_a(before);
		while (true) {
			ValueType type = test_a.next();
			if (type == ValueType::end_of_document) {
				break;
			}
			
			std::cout << (int)type << " ";
			if (type == ValueType::key) {
				std::cout << "(key)";
			}
			else if (type == ValueType::value) {
				std::cout << ("value");
			}
			std::cout << "\n";
		}
	}

	while (true) {

		//str1.clear(true);
		//str2.clear();
		before_map.clear();
		after_map.clear();

		auto x_type = x.next();
		auto y_type = y.next();

		if (x_type == ValueType::end_of_document) {
			break;
		}
		if (y_type == ValueType::end_of_document) {
			break;
		}

		{
			if (x_type == y_type) {
				if (x_type == ValueType::container ||
							x_type == ValueType::end_of_container) {
					x_type = x.next();
					y_type = y.next();
					continue;
				}

				if (*x.get_now() == *y.get_now()) {
					x_type = x.next();
					y_type = y.next();
					continue;
				}
			}

			{
				__x = x;
				__y = y;
	
				size_t count__x = 0;
				size_t count__y = 0;

				// find same value position.
				if (x_type == ValueType::key || x_type == ValueType::value) {
					before_map.insert({ __x.get_now(), count__x });
				}
				if (y_type == ValueType::key || y_type == ValueType::value) {
					after_map.insert({ __y.get_now(), count__y });
				}

				{
					int turn = 1; // 1 -> -1 -> 1 -> -1 ...
					bool pass = false;
					int state = 0;
					int state2 = 0;


					auto __x_type = ValueType::none;
					auto __y_type = ValueType::none;

					while (true) {
						if (__x_type == ValueType::end_of_document) {
							break;
						}
						if (__y_type == ValueType::end_of_document) {
							break;
						}

						if (turn == 1) {
							__x_type = __x.next();
							count__x++;

							if (__x_type == ValueType::key || __x_type == ValueType::value) {
							}
							else {
								state = 1;
								turn *= -1;
								continue;
							}
							state = 0;

							before_map.insert({ __x.get_now(), count__x});

							// if found same value?
							if (after_map.end() != after_map.find(__x.get_now())) {
								pass = true;
								same_value = __x.get_now();
								count__y = after_map.find(__x.get_now())->second;

								if (__y_type == ValueType::key || __y_type == ValueType::value) {
								}
								else {
									state2 = 2;
								}

								break;
							}
						}
						else {
							__y_type = __y.next();
							count__y++;

							if (__y_type == ValueType::key || __y_type == ValueType::value) {
							}
							else
							{
								state2 = 2;

								turn *= -1;
								continue;
							}
							state2 = 0;

							after_map.insert({ __y.get_now(), count__y });

							// if found same value?
							if (before_map.end() != before_map.find(__y.get_now())) {
								pass = true;
								same_value = __y.get_now();
								count__x = before_map.find(__y.get_now())->second;

								if (__x_type == ValueType::key || __x_type == ValueType::value) {
								}
								else {
									state = 1;
								}

								break;
							}
						}

						turn *= -1; // change turn.
					}

					// found same value,
					if (pass) {

						before_x = x;
						if (state == 1) {

							x_type = x.next();

							state = 0;
						}
						before_y = y;
						if (state2 == 2) {

							y_type = y.next();

							state2 = 0;
						}

						temp_x = x;
						temp_y = y;

						line++;

						int state = 0;

						while (*(temp = x.get_now()) != *same_value) {

							result.emplace_back(line, x, x_type, temp_y, y_type, -1, temp->clone());

							before_x = x;

							if (x_type == ValueType::key) {
								x_type = x.next();
								if (x_type == ValueType::value) {
									before_x = x;

									if (*x.get_now() == *same_value) {
										x_type = x.next();
										break;
									}

									x_type = x.next();
								}
							}
							else {
								x_type = x.next();
							}

						}

						state = 0;

						temp_x = before_x;

						while (*(temp = y.get_now()) != *same_value) {

							result.emplace_back(line, temp_x, x_type, y, y_type, +1, temp->clone());

							before_y = y;

							if (y_type == ValueType::key) {
								y_type = y.next();
								if (y_type == ValueType::value) {
									before_y = y;

									if (*y.get_now() == *same_value) {
										y_type = y.next();
										break;
									}

									y_type = y.next();
								}
							}
							else {
								y_type = y.next();
							}
						}
					}
				}
			}
		}
	}


	auto _result = std::move(result);
	result.clear();

	for (size_t i = 0; i < _result.size(); ++i) {
		if (_result[i].type > 0) {
			if (_result[i].y_type == ValueType::key) {
				auto& y = _result[i].y;

				result.emplace_back(_result[i].line, _result[i].x, _result[i].x_type, y, _result[i].y_type, 
										+1, y.get_now()->clone());				
				auto y_type = y.next();
				if (y_type == ValueType::value) {
					result.emplace_back(_result[i].line, _result[i].x, _result[i].x_type, y, y_type, +1, y.get_now()->clone());
				}
			}
			else if (_result[i].y_type == ValueType::value) {
				ClauJsonTraverser& y = _result[i].y;

				if (y.is_in_object()) {
					ClauJsonTraverser temp = y;

					temp.with_key();

					auto temp_type = temp.next();

					result.emplace_back(_result[i].line, _result[i].x, _result[i].x_type, temp, temp_type, +1, y.get_now()->clone());
				}
				result.emplace_back(_result[i].line, _result[i].x, _result[i].x_type,  y, _result[i].y_type, +1, y.get_now()->clone());
			}
			else {
				result.emplace_back(_result[i].line, _result[i].x, _result[i].x_type, _result[i].y, _result[i].y_type, +1, _result[i].data.clone());
			}
		}
		else if (_result[i].type < 0) {
			if (_result[i].x_type == ValueType::key) {
				auto x = _result[i].x;

				result.emplace_back(_result[i].line, x, _result[i].x_type, _result[i].y, _result[i].y_type, - 1, x.get_now()->clone());
			
				auto x_type = x.next();

				if (x_type == ValueType::value) {
					result.emplace_back(_result[i].line, x, x_type, _result[i].y, _result[i].y_type,  - 1, x.get_now()->clone());
				}
			}
			else if (_result[i].x_type == ValueType::value) {
				auto x = _result[i].x;

				if (x.is_in_object()) {
					auto temp = x;
					temp.with_key();
					auto temp_type = temp.next();

					result.push_back(DiffResult{ _result[i].line, temp, temp_type, _result[i].y, _result[i].y_type, - 1, temp.get_now()->clone()});
				}
				result.emplace_back(_result[i].line, x, _result[i].x_type, _result[i].y, _result[i].y_type,  - 1, x.get_now()->clone());
			}
			else {
				result.emplace_back(_result[i].line, _result[i].x, _result[i].x_type, _result[i].y, _result[i].y_type, -1, _result[i].data.clone());
			}
		}
	}

	_result.clear();


	++line;

	// delete -
	if (!x.is_end()) {
		//temp.clear();
		while (true) {
			x.next();
			if (x.is_end()) {
				break;
			}
			result.emplace_back(line, x, ValueType::none, y, ValueType::none, -1, x.get_now()->clone());
		}
	}
	// added +
	if (!y.is_end()) {
		//temp.clear();
		while (true) {
			if (y.is_end()) {
				break;
			}

			result.emplace_back(line, x, ValueType::none, y, ValueType::none, +1, y.get_now()->clone());

			y.next();
		}
	}

	return result;
}

void diff_test2() {
	std::cout << "diff test\n";

	std::string json1 = "{ \"test\" : 1, \"abc\" : [ 1,2,3] }";
	std::string json2 = "{ \"abc\" : [ 2,4,5] }";

	claujson::Document x, y;
	claujson::parser p;
	p.parse_str(json1, x, 0);
	p.parse_str(json2, y, 0);

	std::cout << x.Get() << "\n";
	std::vector<DiffResult> z = Diff(x.Get(), y.Get());
	std::cout << z.size() << "\n";
	for (auto& x : z) {
		std::cout << x.type  << " " << x.data << "\n";
	}
	//int* yy = new int[100];
	//claujson::clean(x);
	//claujson::clean(y);
	//claujson::clean(z);
}
*/

int main(int argc, char* argv[])
{
	{
	//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		std::cout << sizeof(std::vector<std::pair<claujson::_Value, claujson::_Value>>) << "\n";
		//std::cout << sizeof(std::string) << " " << sizeof(claujson::Structured) << " " << sizeof(claujson::Array)
		//	<< " " << sizeof(claujson::Object) << " " << sizeof(claujson::_Value) << "\n";

		if (argc <= 1) {
			std::cout << "[program name] [json file name] (number of thread) \n";
			return 2;
		}

		diff_test();
		std::cout << "----------\n";
		//diff_test2();
		std::cout << "----------\n";
		{
			claujson::Array arr;
			arr.add_element(claujson::_Value(1));
			arr.add_element(claujson::_Value(3));
			arr.insert(1, claujson::_Value(2));
			for (auto& x : arr) {
				std::cout << x << " ";
			}
			std::cout << "\n";
		}
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
				//claujson::StringView s{ "abc", 3 };
				//claujson::StringView x{ "abcg", 4 };

				//std::cout << s.compare(x) << "\n";
				//std::cout << x.compare(s) << "\n";
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


					//return 0;

					auto b = std::chrono::steady_clock::now();
					auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
					std::cout << "total " << dur.count() << "ms\n";
					//	return 0;

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
					std::cout << "clean " << dur.count() << "ms\n";

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
	}
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