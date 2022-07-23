
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

	{

		using claujson::Data;
		Data x(u8"こんにちは wow \n hihi"sv);
		auto& y = x.str_val();
		std::cout << y << "\n";
	}	

	for (int i = 0; i < 3; ++i) {
		claujson::Data j;

		//try
		{
			int a = clock();

			auto x = claujson::Parse(argv[1], 64, j);
			if (!x.first) {
				std::cout << "fail\n";

				claujson::Ptr<claujson::Json> clean(j.as_json_ptr());
				
				return 1;
			}

			int b = clock();
			std::cout << "total " << b - a << "ms\n";

			//claujson::LoadData::save(std::cout, ut);
			//claujson::LoadData::save("output14.json", j);

			int c = clock();
			std::cout << "write " << c - b << "ms\n";

			int counter = 0;
			bool ok = x.first;


			double sum = 0;
			if (true && ok) {
				int chk = 0;
				for (int i = 0; i < 1; ++i) {
					if (j.is_ptr()) {
						auto& features = j.as_object()[1]; // j[1];
						for (auto& feature : features.as_array()) {
							auto& geometry = feature.as_object().at("geometry"sv); // as_array()[t].as_object()["geometry"];
							auto& coordinates = geometry.as_object().at("coordinates"sv);
							auto& coordinate = coordinates.as_array()[0];
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

			int c1 = clock();

			claujson::LoadData::save_parallel("total_end.json", j, 0);

			//std::cout << "\ncat \n";
			//system("cat total_end.json");
			//std::cout << "\n";

			int c2 = clock();
			std::cout << "\nwrite " << c2 - c1 << "ms\n";


			sum = 0; counter = 0; 
			if (false && ok) {
				int chk = 0;
				for (int i = 0; i < 1; ++i) {
					auto& features = j.as_json()[1]; // j[1];
					for (auto& feature : features.as_array()) {
						auto& geometry = feature.as_object().at("geometry"sv); // as_array()[t].as_object()["geometry"];
						if (geometry.is_ptr()) { // is_obj or arr?
							auto& coordinates = geometry.as_object().at("coordinates"sv);
							auto& coordinate = coordinates.as_array()[0];
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
			std::cout << clock() - c2 << "ms\n";
			std::cout << "Re.. " << sum << " " << counter << "\n";
			
			claujson::Ptr<claujson::Json> clean(j.as_json_ptr());
			
			return !ok;
		}
		/*catch (...) {
			if (j.is_ptr() && j.ptr_val()) {
				claujson::Ptr<claujson::Json> clean(&j.as_json());
			}

			std::cout << "internal error\n";
			return 1;
		}*/
	}

	return 0;
}


