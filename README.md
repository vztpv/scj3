# claujson
experimental parallel json parser, (using simdjson, thread)

scanning - simd,

parsing - multi-thread(used in clauparser),

save - multi-thread
  
(C++14~, 64bit)

# Use static lib (claujson) with CMake!
https://github.com/vztpv/scj3_cmake_test

# Usage... with citylots.json

```c++
//claujson::init(0); // //
claujson::parser p;
claujson::writer w;
claujson::Document d;
bool ok;

auto x = p.parse(argv[1], d, 0); // filename, Value, thread_num...

claujson::_Value& j = d.Get();

if (!x.first) {
	std::cout << "fail\n";
	//claujson::clean(j);
	return 1;
}

//claujson::save("test12.txt", j);
w.write_parallel("test34.json", j, 0);

int counter = 0;
ok = x.first;

double sum = 0;

static const auto& _geometry = claujson::_Value("geometry"sv);
static const auto& _coordinates = claujson::_Value("coordinates"sv);

if (true && ok) {
    for (int i = 0; i < 1; ++i) {
        if (j.is_structured()) {
            auto& features = j[1];
            claujson::Array* features_arr = features.as_array(); // as_array_ptr() ?
            if (!features_arr) {
                continue;
            }
            for (auto& feature : *features_arr) { // feature["geometry"sv] <- no utf-8 str chk?, at("geometry"sv) : check valid utf-8 str?
                auto& coordinate = feature[_geometry][_coordinates][0]; 
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
                        }
                    }
                }
            }
        }
    }
}

std::cout << sum << " ";
std::cout << counter << "  ";

//claujson::clean(j);

```

# Use CMake, (msvc -> use Release, 64bit), tested with mimalloc

# fmt, progschj/ThreadPool (some modified to use C++17)
# simdjson (some modified to use it as Scanner)

# Using simdjson/simdjson (https://github.com/simdjson/simdjson) MIT License
Copyright 2018-2025 The simdjson authors

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# fmtlib/fmt
	Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
	
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
	--- Optional exception to the license ---
	
	As an exception, if, as a result of your compiling your source code, portions
	of this Software are embedded into a machine-executable object form of such
	source code, you may redistribute such embedded portions in such object form
	without including the above copyright and permission notices.

# progschj/ThreadPool zlib License
	Copyright (c) 2012 Jakob Progsch, Václav Zeman
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.
	
	Permission is granted to anyone to use this software for any purpose,
	including commercial appiolicatns, and to alter it and redistribute it
	freely, subject to the following restrictions:
	
	   1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.

	   2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	
	   3. This notice may not be removed or altered from any source
	   distribution.
