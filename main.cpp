
// now, test only haswell..
// need C++17, 64bit..

#define _CRT_SECURE_NO_WARNINGS

#include "mimalloc-new-delete.h"

#include <iostream>
#include <string>
#include <ctime>


#include "claujson.h" // using simdjson 2.0.0

#include <cstring>

using namespace std::literals::string_view_literals;


int main(int argc, char* argv[])
{
	
	for (int i = 0; i < 3; ++i) {
		claujson::Data j;
		
		try {
			int a = clock();
			
			auto x = claujson::Parse(argv[1], 64, j);
			if (!x.first) {
				std::cout << "fail\n";
				return 1;
			}

			int b = clock();
			std::cout << "total " << b - a << "ms\n";

			//claujson::LoadData::save(std::cout, ut);
			//claujson::LoadData::save("output13.json", *((claujson::Json*)j.ptr_val()));
			
			int c = clock();
			std::cout << c - b << "ms\n";

int counter = 0;
			bool ok = x.first;
			

				double sum = 0;
				if (ok) {
					int chk = 0;
					for (int i = 0; i < 1; ++i) {
						auto& A = j.as<claujson::Json>()[1]; // j[1];
						for (auto& features : A.as<claujson::Array>()) {
							auto& y = features.as<claujson::Object>().at("geometry"sv); // as_array()[t].as_object()["geometry"];
							if (y.is_ptr()) { // is_obj or arr?
								auto& yyy = y.as<claujson::Object>().at("coordinates"sv);
								auto& yyyy = yyy.as<claujson::Array>()[0];
								for (auto& temp : yyyy.as<claujson::Array>()) {
									for (auto& x : temp.as<claujson::Array>()) {
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
			
			claujson::Ptr<claujson::Json> clean(&j.as<claujson::Json>());
			clean.clear();
			return !ok;
		}
		catch (...) {
			if (j.is_ptr() && j.ptr_val()) {
				claujson::Ptr<claujson::Json> clean(&j.as<claujson::Json>());
			}

			std::cout << "internal error\n";
			return 1;
		}
	}

	return 0;
}

