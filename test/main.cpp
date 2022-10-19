
#include <iostream>
#include <string>
#include <vector>

#include "mimalloc-new-delete.h" // ÇÊ¼ö!
#include "claujson.h"
#include "simdjson.h"

using namespace std::string_view_literals;

// 0 ~ 11 : type
inline std::string make_json_str(int type) {
	std::string result;

	switch (type) {
	case 0:
		result = " \" test \" ";
		break;
	case 1:
		result = " 0 ";
		break;
	case 2:
		result = " 1.5 ";
		break;
	case 3:
		result = " true ";
		break;
	case 4:
		result = " false ";
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
		_stack.push_back(0); // 0 ~ 11
	}

	while (!_stack.empty()) {
		if (_stack.size() == element_max_num) {
			std::string str;
			
			for (int i = 0; i < _stack.size(); ++i) {
				str += make_json_str(_stack[i]);
			}

			result.push_back(str);
		}

		if (_stack.back() < 12) {
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
				_stack.push_back(0);
			}
		}
	}

	return result;
}


int main(int argc, char* argv[])
{
	claujson::init();
	claujson::log.no_print();

	int count = 0;
	for (int i = 1; i < 6; ++i) {
		auto test = new_json_str(i);
		//std::vector<std::string> test = { std::string("  ]   ,  [  ]  }  ") }; // internal error..

		for (const auto& str : test) {
			{
				std::cout << count << "\t";
				std::cout << str << "\n";
				claujson::Data ut;

				
				bool cj_error = !claujson::parse_str(str, ut, 1).first;
				std::cout << cj_error << "\n";
				claujson::clean(ut);

				_simdjson::dom::parser x;
				auto y = x.parse(str);
				count++;
				if (cj_error == (y.error() == _simdjson::SUCCESS)) {
					std::cout << "count " << count << " ";
					std::cout << cj_error << " // " <<  str << "\n // " << (int)y.error() << "\n";
					return -1;
				}
			}
		}
	}

	std::cout << "end";

	return 0;
}

