
// now, test only haswell..
// need C++14, 64bit..


#include "mimalloc-new-delete.h"


#include <iostream>
#include <string>
#include <ctime>

#include "claujson.h" // using simdjson 3.1.1

#include "_simdjson.h"

#include <cstring>

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
		})"sv;

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
		})"sv;

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

		JsonIterator& iterate(std::function<void(Value&)> func) {
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

		Value& now() {
			if (_m_now) {
				return _m_now->get_value_list(_m_child_pos_in_parent.back());
			}
			static Value null_data(nullptr, false);
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

namespace test {
	class JsonIterator {
	public:
		JsonIterator& enter() {
			if (_m_now && _m_now->get_value_list(_m_child_pos_in_parent.back()).is_structured()) {
				_m_now = _m_now->get_value_list(_m_child_pos_in_parent.back()).as_structured_ptr();
				_m_child_pos_in_parent.push_back(0);
			}
			return *this;
		}

		JsonIterator& quit() {
			if (_m_now) {
				_m_child_pos_in_parent.pop_back();
				_m_now = _m_now->get_parent();
			}
			return *this;
		}

		JsonIterator& next() {
			if (_m_now) {
				_m_child_pos_in_parent.back()++;
			}
			return *this;
		}

		bool is_next_valid() const {
			return _m_now && _m_child_pos_in_parent.empty() == false && _m_child_pos_in_parent.back() < _m_now->get_data_size();
		}

		const claujson::Value& now_value() {
			if (_m_now) {
				return _m_now->get_value_list(_m_child_pos_in_parent.back()); // get_value_list_ex..? check valid of index?
			}
			static claujson::Value null_data(nullptr, false);
			return null_data;
		}

		const claujson::Value& now_key() {
			if (_m_now) {
				return _m_now->get_key_list(_m_child_pos_in_parent.back()); // get_value_list_ex..? check valid of index?
			}
			static claujson::Value null_data(nullptr, false);
			return null_data;
		}

	public:
		JsonIterator(const claujson::Structured* arr_or_obj) : _m_now(arr_or_obj) {
			_m_child_pos_in_parent.push_back(0);
		}
	private:
		const claujson::Structured* _m_now = nullptr;
		std::vector<size_t> _m_child_pos_in_parent;
	};
	
	class StrStream {
	private:
		fmt::memory_buffer out;

	public:

		const char* buf() const {
			return out.data();
		}
		size_t buf_size() const {
			return out.size();
		}

		StrStream& operator<<(const char* x) {
			fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(StringView sv) { // chk! 
			if (sv.empty() || sv[0] == '\0') {
				return *this;
			}

			fmt::format_to_n(std::back_inserter(out), sv.size(), "{}", sv.data());

			return *this;
		}

		StrStream& operator<<(double x) {
			if (x == 0.0) {
				fmt::format_to(std::back_inserter(out), "0.0"); //?
			}
			else {
				fmt::format_to(std::back_inserter(out), "{}", x); // FMT_COMPILE("{:.10f}"), x);
			}
			return *this;
		}

		StrStream& operator<<(int64_t x) {
			fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(uint64_t x) {
			fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(char ch) {
			fmt::format_to(std::back_inserter(out), "{}", ch);
			return *this;
		}
	};

	inline void write_char(StrStream& stream, char ch) {
		switch (ch) {
		case '\\':
			stream << "\\\\";
			break;
		case '\"':
			stream << "\\\"";
			break;
		case '\n':
			stream << "\\n";
			break;
		case '\b':
			stream << "\\b";
			break;
		case '\f':
			stream << "\\f";
			break;
		case '\r':
			stream << "\\r";
			break;
		case '\t':
			stream << "\\t";
			break;
		default:
		{
			int code = ch;
			if ((code >= 0 && code < 0x20) || code == 0x7F)
			{
				char buf[] = "\\uDDDD";
				snprintf(buf + 2, 5, "%04X", code);
				stream << buf;
			}
			else {
				stream << ch;
			}
		}
		}
	}

	inline void write_char(StrStream& stream, const std::string& str) {
		size_t sz = str.size();
		for (size_t i = 0; i < sz; ++i) {
			write_char(stream, str[i]);
		}
	}

	void _save_primitive(StrStream& stream, const claujson::Value& x) {
		switch (x.type()) {
		case claujson::ValueType::STRING: {
			stream << "\"";

			size_t len = x.str_val().size();
			//for (uint64_t j = 0; j < len; ++j) {
			write_char(stream, x.str_val());
			//}
			stream << "\"";

		}break;
		case claujson::ValueType::BOOL: {
			stream << (x.bool_val() ? "true" : "false");
		}break;
		case claujson::ValueType::FLOAT: {
			stream << (x.float_val());
		}break;
		case claujson::ValueType::INT: {
			stream << x.int_val();
		}break;
		case claujson::ValueType::UINT: {
			stream << x.uint_val();
		}break;
		case claujson::ValueType::NULL_: {
			stream << "null";
		}break;
		}
	}
	void _save(StrStream& stream, const claujson::Value& j) {
		if (j.is_primitive()) {
			return _save_primitive(stream, j);
		}
		std::vector<claujson::ValueType> _array_or_obj;
		JsonIterator iter(j.as_structured_ptr());
		bool is_j_array = false;

		if (iter.now_value().is_array()) {
			stream << "[";
			_array_or_obj.push_back(claujson::ValueType::ARRAY);
			is_j_array = true;
		}
		else {
			stream << "{";
			_array_or_obj.push_back(claujson::ValueType::OBJECT);
		}

		do {
			if (iter.is_next_valid()) {
				if (_array_or_obj.back() == claujson::ValueType::ARRAY) {
					if (auto& x = iter.now_value(); x.is_array()) {
						stream << "[";
						_array_or_obj.push_back(claujson::ValueType::ARRAY);
						iter.enter();
					}
					else if (x.is_object()) {
						stream << "{";
						_array_or_obj.push_back(claujson::ValueType::OBJECT);
						iter.enter();
					}
					else {
						_save_primitive(stream, x);

						iter.next();

						if (iter.is_next_valid()) {
							stream << ",";
						}
					}
				}
				else {
					auto& key = iter.now_key();

					_save_primitive(stream, key);

					stream << ":";

					if (auto& x = iter.now_value(); x.is_array()) {
						stream << "[";
						_array_or_obj.push_back(claujson::ValueType::ARRAY);
						iter.enter();
					}
					else if (x.is_object()) {
						stream << "{";
						_array_or_obj.push_back(claujson::ValueType::OBJECT);
						iter.enter();
					}
					else {
						_save_primitive(stream, x);

						iter.next();

						if (iter.is_next_valid()) {
							stream << ",";
						}
					}
				}
			}
			else {
				iter.quit();
				if (_array_or_obj.back() == claujson::ValueType::ARRAY) {
					stream << "]";
				}
				else { // x.is_object()
					stream << "}";
				}
				_array_or_obj.pop_back();

				iter.next();

				if (iter.is_next_valid()) {
					stream << ",";
				}
			}
		} while (!_array_or_obj.empty());
	
		if (is_j_array) {
			stream << "]";
		}
		else {
			stream << "}";
		}
	}
	void save(const std::string& fileName, const claujson::Value& j) {
		StrStream stream;

		_save(stream, j);

		std::ofstream outFile;
		outFile.open(fileName, std::ios::binary); // binary!
		if (outFile) {
			outFile.write(stream.buf(), stream.buf_size());
			outFile.close();
		}
	}

}
int main(int argc, char* argv[])
{
	std::cout << sizeof(std::string) << " " << sizeof(claujson::Structured) << " " << sizeof(claujson::Array)
		<< " " << sizeof(claujson::Object) << " " << sizeof(claujson::Value) << "\n";

	if (argc <= 1) {
		std::cout << "[program name] [json file name] (thr_num) \n";
		return 2;
	}

	/*
	std::cout << sizeof(claujson::Value) << "\n";
	std::cout << sizeof(claujson::Array) << "\n";
	std::cout << sizeof(claujson::Object) << "\n";
	{
		auto str = R"()"sv;

		claujson::init(0);

		claujson::Value ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto str = R"("A" : 3 )"sv;

		claujson::init(0);

		claujson::Value ut;
		std::cout << claujson::parse_str(str, ut, 1).first << "\n";
	}

	{
		auto str = R"(3,  3)"sv;

		claujson::init(0);

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

		if (argc < 4) {
			claujson::log.console();
			claujson::log.info();
			claujson::log.warn();
		}

		//claujson::log.no_print();
		//claujson::log.console();
		//claujson::log.info();
		//claujson::log.warn();
		//claujson::log.info(true);
		//claujson::log.warn(true);

		//utf_8_test();

		//key_dup_test();

		//json_pointer_test();

		//str_test();

		for (int i = 0; i < 3; ++i) {
			claujson::Value j;
			bool ok;
			//try
			{

				auto a = std::chrono::steady_clock::now();

				int thr_num = 0;

				if (argc > 2) {
					thr_num = std::atoi(argv[2]);
				}

				// not-thread-safe..
				auto x = claujson::parse(argv[1], j, thr_num, true); // argv[1], j, 64 ??

				if (!x.first) {
					std::cout << "fail\n";

					claujson::clean(j);

					return 1;
				}

				auto b = std::chrono::steady_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
				std::cout << "total " << dur.count() << "ms\n";

				//
				//claujson::clean(j);

				//return 0;
				//
							//	claujson::save("test12.txt", j);
				//claujson::save_parallel("test34.json", j, thr_num);
				claujson::save_parallel("test56.json", j, 0, false);
				std::cout << "save_parallel " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - b).count() << "ms\n";

				//b = std::chrono::steady_clock::now();
				//test::save("test78.json", j);

			//	std::cout << "save " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - b).count() << "ms\n";


				claujson::clean(j);

				return 0;

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
							claujson::Array* features_arr = features.as_array();
							if (!features_arr) {
								continue;
							}
							for (auto& feature : *features_arr) { // feature["geometry"sv] <- no utf-8 str chk?, at("geometry"sv) : check valid utf-8 str?
								auto& coordinate = feature["geometry"sv]["coordinates"sv][0];  // feature.json_pointerB(vec)[0];  
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

				{
					double sum = 0;
					counter = 0;
					
					if (true && ok) {
						int chk = 0;
						for (int i = 0; i < 1; ++i) {
							claujson::JsonIterator iter(j[1].as_structured_ptr()); // features
							while (iter.is_valid()) {
								auto& x = iter.now()["geometry"sv]["coordinates"sv][0]; // coordinate
								claujson::JsonIterator iter2(x.as_structured_ptr());
								iter2.iterate([&](claujson::Value& v) {
									claujson::JsonIterator iter3(v.as_structured_ptr());
									iter3.iterate([&](claujson::Value& v) {
										if (v.is_float()) {
											sum += v.get_floating();
											counter++;
											chk++;
										}
										}
									);
									}
								);
								iter.next();
							}
						}
					}

					auto d = std::chrono::steady_clock::now();
					dur = std::chrono::duration_cast<std::chrono::milliseconds>(d - c);
					std::cout << dur.count() << "ms\n";
					std::cout << sum << " ";
					std::cout << counter << "  ";
				}
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


