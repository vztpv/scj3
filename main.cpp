
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
		Data x("wow \n hihi");
		std::cout << x.str_val(); //
	}


	for (int i = 0; i < 3; ++i) {
		claujson::Data j;

		//try
		{
			int a = clock();

			auto x = claujson::Parse(argv[1], 64, j);
			if (!x.first) {
				std::cout << "fail\n";

				if (j.is_ptr() && j.ptr_val()) {
					claujson::Ptr<claujson::Json> clean(&j.as<claujson::Json>());
				}
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
					if (j && j.is_ptr()) {
						auto& features = j.as<claujson::Object>()[1]; // j[1];
						for (auto& feature : features.as<claujson::Array>()) {
							auto& geometry = feature.as<claujson::Object>().at("geometry"sv); // as_array()[t].as_object()["geometry"];
							auto& coordinates = geometry.as<claujson::Object>().at("coordinates"sv);
							auto& coordinate = coordinates.as<claujson::Array>()[0];
							for (auto& coordinate_ : coordinate.as<claujson::Array>()) {
								for (auto& x : coordinate_.as<claujson::Array>()) {
									if (x && x.is_float()) {
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

			int c2 = clock();
			std::cout << "\nwrite " << c2 - c1 << "ms\n";


			sum = 0; counter = 0; 
			if (true && ok) {
				int chk = 0;
				for (int i = 0; i < 1; ++i) {
					auto& features = j.as<claujson::Json>()[1]; // j[1];
					for (auto& feature : features.as<claujson::Array>()) {
						auto& geometry = feature.as<claujson::Object>().at("geometry"sv); // as_array()[t].as_object()["geometry"];
						if (geometry.is_ptr()) { // is_obj or arr?
							auto& coordinates = geometry.as<claujson::Object>().at("coordinates"sv);
							auto& coordinate = coordinates.as<claujson::Array>()[0];
							for (auto& coordinate_ : coordinate.as<claujson::Array>()) {
								for (auto& x : coordinate_.as<claujson::Array>()) {
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
			if (j.is_ptr() && j.ptr_val()) {
				claujson::Ptr<claujson::Json> clean(&j.as<claujson::Json>());
			}

			return !ok;
		}
		/*catch (...) {
			if (j.is_ptr() && j.ptr_val()) {
				claujson::Ptr<claujson::Json> clean(&j.as<claujson::Json>());
			}

			std::cout << "internal error\n";
			return 1;
		}*/
	}

	return 0;
}


