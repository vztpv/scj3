
// now, test only haswell..
// need C++17, 64bit..

#define _CRT_SECURE_NO_WARNINGS

#include <vld.h>
#include "mimalloc-new-delete.h"

#include <iostream>
#include <string>
#include <ctime>


#include "claujson.h" // using simdjson 2.0.0

#include <cstring>

using namespace std::literals::string_view_literals;


int main(int argc, char* argv[])
{
	
	for (int i = 0; i < 64; ++i) {
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
			/*if (ok) {
				int chk = 0;
				for (int i = 0; i < 1; ++i) {
					auto _j = (*((claujson::Json*)j.ptr_val()))[0];
					auto& A = (*((claujson::Json*)_j.ptr_val()))[1]; // j[1];

					for (auto& features : (*((claujson::Array*)A.ptr_val()))) {

						auto& y = ((claujson::Json*)features.ptr_val())->at("geometry"sv); // as_array()[t].as_object()["geometry"];
						
						
						if (y.is_ptr()) {
							auto& yyy = ((claujson::Json*)y.ptr_val())->at("coordinates"sv);
							//if (yyy)
							{
								auto& yyyy = (*((claujson::Json*)yyy.ptr_val()))[0];
								//	if (yyyy)
								{
									for (auto& temp : (*((claujson::Array*)yyyy.ptr_val()))) {
										for (auto& x : (*((claujson::Array*)temp.ptr_val()))) {

											if (x.is_float()) {
												sum += x.float_val();

												counter++;
												chk++;
											}

										}
									}

								}
							}
							//	//std::cout << dur.count() << "ns\n";

						}


					}
				}
			}

			std::cout << clock() - c << "ms\n";
			std::cout << sum << " ";
			std::cout << counter << " ";	int chk = 0;
				for (int i = 0; i < 1; ++i) {
					auto _j = (*((claujson::Json*)j.ptr_val()))[0];
					auto& A = (*((claujson::Json*)_j.ptr_val()))[1]; // j[1];

					for (auto& features : (*((claujson::Array*)A.ptr_val()))) {

						auto& y = ((claujson::Json*)features.ptr_val())->at("geometry"sv); // as_array()[t].as_object()["geometry"];
						
						
						if (y.is_ptr()) {
							auto& yyy = ((claujson::Json*)y.ptr_val())->at("coordinates"sv);
							//if (yyy)
							{
								auto& yyyy = (*((claujson::Json*)yyy.ptr_val()))[0];
								//	if (yyyy)
								{
									for (auto& temp : (*((claujson::Array*)yyyy.ptr_val()))) {
										for (auto& x : (*((claujson::Array*)temp.ptr_val()))) {

											if (x.is_float()) {
												sum += x.float_val();

												counter++;
												chk++;
											}

										}
									}

								}
							}
							//	//std::cout << dur.count() << "ns\n";

						}


					}
				}
			}

			std::cout << clock() - c << "ms\n";
			std::cout << sum << " ";
			std::cout << counter << " ";
			*/
			claujson::Ptr<claujson::Json> clean(claujson::Ptr((claujson::Json*)j.ptr_val()));

			return !ok;
		}
		catch (...) {
			std::cout << "internal error\n";
			return 1;
		}
	}
	
	//getchar();

	return 0;
}

