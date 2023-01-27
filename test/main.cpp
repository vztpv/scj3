
#include <iostream>
#include <string>
#include <vector>

#include <vld.h>

//#include "mimalloc-new-delete.h" // ÇÊ¼ö!
#include "claujson.h"
#include "simdjson.h"

using namespace std::string_view_literals;


class AAA {
public:
	union {
		claujson::Array* arr;
		claujson::Object* obj;
		uint64_t x;
	};
	int type;

	AAA() : x(0), type(2) {
		//
	}
};

class BBB {
public:

	union {
		claujson::Structured* json;
		uint64_t x;
	};
	int type;

	BBB() : x(0), type(1) {
		//
	}
};


int AAA_test(const std::string& str) {
	std::vector<AAA> _stack;

	for (int i = 0; i < str.size(); ++i) {
		if (str[i] == '[') {
			AAA temp;
			temp.type = 0;
			temp.arr = new claujson::Array();
			_stack.push_back(temp);
		}
		else if (str[i] == '{') {
			AAA temp;
			temp.type = 1;
			temp.obj = new claujson::Object();
			_stack.push_back(temp);
		}
		else if (str[i] == ']') {
			AAA temp = _stack.back();
			_stack.pop_back();
			
			if (temp.type == 0) {
				delete temp.arr;
			}
			else {
				delete temp.obj;
			}
		}
		else {
			AAA temp = _stack.back();
			_stack.pop_back();

			if (temp.type == 0) {
				delete temp.arr;
			}
			else {
				delete temp.obj;
			}
		}
	}
	return _stack.size();
}
int BBB_test(const std::string& str) {
	std::vector<BBB> _stack;

	for (int i = 0; i < str.size(); ++i) {
		if (str[i] == '[') {
			BBB temp;
			temp.type = 0;
			temp.json = new claujson::Array();
			_stack.push_back(temp);
		}
		else if (str[i] == '{') {
			BBB temp;
			temp.type = 1;
			temp.json = new claujson::Object();
			_stack.push_back(temp);
		}
		else if (str[i] == ']') {
			BBB temp = _stack.back();
			_stack.pop_back();

			delete temp.json;
		}
		else {
			BBB temp = _stack.back();
			_stack.pop_back();

			delete temp.json;
		}
	}
	return _stack.size();
}


void union_test() {
	int a = 0, b = 0;

	std::string str;

	str = "[]{}[]{}";

	for (int i = 0; i < 23; ++i) {
		str = str + str;
	}

	str = "[" + str + "]";

	a = clock();
	int x = AAA_test(str);
	b = clock();
	std::cout << b - a << "ms\n";
	a = clock();
	int y = BBB_test(str);
	b = clock();
	std::cout << b - a << "ms\n";

	std::cout << x << " " << y << "\n";
}



// 0 ~ 11 : type
inline std::string make_json_str(int type) {
	std::string result;

	switch (type) {
	case 4:
		result = " \" test \" ";
		break;
	case 5:
		result = " null ";
		break;
	case 6:
		result = " [ ";
		break;
	case 7:
		result = " ] ";
		break;
	case 8:
		result = " { ";
		break;
	case 9:
		result = " } ";
		break;
	case 10 :
		result = " : ";
		break;
	case 11:
		result = " , ";
	}

	return result;
}

std::vector<std::string> new_json_str(const int element_max_num) {
	std::vector<std::string> result;
	int now_depth = 0;
	std::vector<int> _stack;

	for (int i = 0; i < element_max_num; ++i) {
		_stack.push_back(4); // 4 ~ 11
	}

	while (!_stack.empty()) {
		if (_stack.size() == element_max_num) {
			std::string str;

			for (int i = 0; i < _stack.size(); ++i) {
				str += make_json_str(_stack[i]);
			}

			result.push_back(str);
		}

		if (_stack.back() + 1 < 12) {
			_stack.back()++;
		}
		else {
			do {
				_stack.pop_back();
				if (_stack.empty()) {
					return result;
				}
				_stack.back()++;
				
			} while (_stack.back() >= 12);

			for (int i = _stack.size(); i < element_max_num; ++i) {
				_stack.push_back(4);
			}
		}
	}

	return result;

}


std::vector<std::string> new_json_str2(const int element_max_num) {
	std::vector<std::string> result;
	int now_depth = 0;
	std::vector<int> _stack;

	for (int i = 0; i < element_max_num; ++i) {
		_stack.push_back(4); // 4 ~ 11
	}

	std::vector<int> _stack_test;

	while (!_stack.empty()) {
		if (_stack.size() == element_max_num) {

			int state = 0;

			switch (_stack[0]) {
			case 6: // [
				if(_stack.back() == 7) { // }
					state = 1;
				}
			case 8: // {
				if (_stack.back() == 9) { // } 
					state = 1;
				}
				break;
			}
			
			bool pass = false;

			for (int i = 0; i < _stack.size(); ++i) {
				if (_stack[i] == 6 || _stack[i] == 8) {
					_stack_test.push_back(_stack[i]);
				}
				if (_stack[i] == 7 && _stack_test.empty() == false && _stack_test.back() == 6) {
					pass = true;
					break;
				}
				if (_stack[i] == 9 && _stack_test.empty() == false && _stack_test.back() == 8) {
					pass = true;
					break;
				}
			}

			if (pass == false) {
				std::string str;

				for (int i = 0; i < _stack.size(); ++i) {
					str += make_json_str(_stack[i]);
				}

				result.push_back(str);
			}
		}

		if (_stack.back() + 1 < 12) {
			_stack.back()++;
		}
		else {
			do {
				_stack.pop_back();
				if (_stack.empty()) {
					return result;
				}
				_stack.back()++;

			} while (_stack.back() >= 12);

			for (int i = _stack.size(); i < element_max_num; ++i) {
				_stack.push_back(4);
			}
		}
	}

	return result;

}

int main(int argc, char* argv[])
{
	claujson::init();
	//claujson::log.console();

	claujson::Value ut;
	claujson::parse_str("{}", ut, 0);
	
	clean(ut);
	{
	//	union_test();

		//return 0;
	}

	int count = 0;
	int a = clock();
	int k = a;
	for (int i = 1; i < 8; ++i) {
		auto test = new_json_str(i);

		//std::cout << "test size : " << test.size() << "\n";
		//std::vector<std::string> test = { std::string("{ \" test \" : null , \" test \" : \" test \", \" test \" : null }") }; // internal error..

		for (auto& _str : test) {
			std::string str =  "{ \" test \" : null , " + _str + ", \" test \" : null }";

			int b = clock();

			if ((b - a) > 2000) {
				std::cout << "\r" << b - k << "ms, count : " << count << " " << (1000.0* count / (b-k)) << " count/s";
				a = b;
			}
			{
				//std::cout << count << "\t";
				//std::cout << str << "\n";
				
				for (int j = 0; j < 2; ++j) {
					claujson::Value ut;

					int first_count = VLDGetLeaksCount();

					bool cj_error = !claujson::parse_str(str, ut, j).first;
					//std::cout << cj_error << "\n";
					claujson::clean(ut);


					if (VLDGetLeaksCount() - first_count != 0) {
						std::cout << count << " " << str << "memory leak..\n";
						return -2;
					}

					//VLDDisable();


					_simdjson::dom::parser x;
					auto y = x.parse(str);
					count++;
					if (cj_error == (y.error() == _simdjson::SUCCESS)) {
						std::cout << "count " << count << " ";
						std::cout << cj_error << " // " << str << "\n // " << (int)y.error() << "\n";
						return -1;
					}
				}
			}
		}
	}

	std::cout << "\nend\n";

	return 0;
}

