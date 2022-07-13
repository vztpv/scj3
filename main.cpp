
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

inline claujson::Json* Divide(int n, claujson::Data& j, claujson::Json*& result, int& hint) {
	size_t len = claujson::LoadData::Size(&j.as<claujson::Json>());
	size_t len_x = len / n;

	hint = 0;
	claujson::Json* temp = nullptr;
	claujson::LoadData::Find(&j.as<claujson::Json>(), len_x, temp, hint);
	claujson::LoadData::Divide(temp, result);
	
	return temp;
}


int main(int argc, char* argv[])
{

	for (int i = 0; i < 3; ++i) {
		claujson::Data j;

		 {
			int a = clock();

			auto x = claujson::Parse(argv[1], 64, j);
			if (!x.first) {
				std::cout << "fail\n";

				claujson::Ptr<claujson::Json> clean(&j.as<claujson::Json>());
				return 1;
			}

			int b = clock();
			std::cout << "total " << b - a << "ms\n";

			//claujson::LoadData::save(std::cout, ut);
			claujson::LoadData::save("output14.json", j);

			int c = clock();
			std::cout << "write " << c - b << "ms\n";

			int counter = 0;
			bool ok = x.first;


			double sum = 0;
			if (ok) {
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

			std::cout << clock() - c << "ms\n";
			std::cout << sum << " ";
			std::cout << counter << "  ";

			int c1 = clock();

			std::vector<claujson::Json*> temp(8, nullptr); //

			{
				std::vector<claujson::Json*> result(temp.size(), nullptr);

				std::vector<int> hint(temp.size(), false);

				std::vector<int> arr{ 9, 8, 7, 6, 5, 4, 3, 2 }; // chk!

				std::vector<claujson::Data> data(temp.size());

				result[0] = &j.as<claujson::Json>(); 
				hint[0] = false;

				temp[0] = Divide(arr[0], j, result[1], hint[0]);

				for (int i = 1; i < temp.size() - 1; ++i) {
					claujson::Data data(result[i]);
					temp[i] = Divide(arr[i], data, result[i + 1], hint[i]);
				}


				std::vector<claujson::LoadData::StrStream> stream(temp.size());

				std::vector<std::thread> thr(temp.size());
				

				thr[0] = std::thread(claujson::LoadData::save_, std::ref(stream[i]), claujson::Data(result[0]), temp[0], (false));
				for (size_t i = 1; i < thr.size(); ++i) {
					thr[i] = std::thread(claujson::LoadData::save_, std::ref(stream[i]), claujson::Data(result[i]), temp[i], (hint[i - 1]));
				}

				for (size_t i = 0; i < thr.size(); ++i) {
					thr[i].join();
				}

				std::ofstream outFile("total_end.json", std::ios::binary);

				for (size_t i = 0; i < stream.size(); ++i) {
					outFile.write(stream[i].buf(), stream[i].buf_size());
				}
				
				outFile.close();

				//claujson::LoadData::save("first.json", j);
				//claujson::LoadData::save("second.json", k, hint);
			

				int ret = claujson::LoadData::Merge2(temp[0]->get_parent().get(), result[1], &temp[1]);
				for (size_t i = 1; i < thr.size() - 1; ++i) {
					int ret = claujson::LoadData::Merge(temp[i]->get_parent().get(), result[i + 1], &temp[i + 1]);
				}


				//claujson::LoadData::save("total.json", j);

				for (size_t i = 1; i < result.size(); ++i) {
					claujson::Ptr<claujson::Json> clean2(result[i]);
				}
			}

			int c2 = clock();
			std::cout << "\nwrite " << c2 - c1 << "ms\n";

			claujson::Ptr<claujson::Json> clean(&j.as<claujson::Json>());
			
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


