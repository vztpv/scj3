# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.11.0)

# 프로젝트 정보
project(
  claujson
  VERSION 0.21.1 #
  DESCRIPTION "claujson"
  LANGUAGES CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")

#set(CMAKE_CXX_STANDARD 17) # C++17
set(CMAKE_BUILD_TYPE "Release")

set(LIB_NAME claujson) # static lib
set(LIB_NAME14 claujson14) # static lib with c++ 14

set(ROOT .)

set(LIB_DIR ${ROOT})

set(SOURCE_DIR ${ROOT})

set(INCLUDE_DIR ${ROOT}) # ${ROOT}/_deps/rapidjson-src/include)


# build setting.
add_library(${LIB_NAME} STATIC ${SOURCE_DIR}/claujson.cpp ${SOURCE_DIR}/_simdjson.cpp) 
add_library(${LIB_NAME14} STATIC ${SOURCE_DIR}/claujson.cpp ${SOURCE_DIR}/_simdjson.cpp) 
 
include(FetchContent)
FetchContent_Declare(
  RapidJSON
  GIT_REPOSITORY "https://github.com/Tencent/rapidjson"
  GIT_TAG "v1.1.0" # v1.1.0
  )
FetchContent_GetProperties(RapidJSON)
if(NOT RapidJSON_POPULATED)
	FetchContent_Populate(RapidJSON)
	set(gtest_build_test OFF CACHE BOOL "Enable gtest tests" FORCE)
endif()

# header..
target_include_directories(${LIB_NAME}  PUBLIC ${INCLUDE_DIR} PRIVATE ${rapidjson_SOURCE_DIR}/include)
target_include_directories(${LIB_NAME14} PUBLIC ${INCLUDE_DIR} PRIVATE ${rapidjson_SOURCE_DIR}/include)


if (UNIX) 
        target_compile_options(${LIB_NAME} PRIVATE -m64 -std=c++20 -march=native -pthread -Wno-narrowing -Wextra -O2)
        target_compile_options(${LIB_NAME14} PRIVATE -m64 -std=c++14 -march=native -pthread -Wno-narrowing -Wextra -O2)
      
elseif (MSVC)

target_compile_definitions(${LIB_NAME} PRIVATE
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "UNICODE;"
        "_UNICODE"
    )
	
	if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${LIB_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /std:c++20;
                /Oi;
                /Gy
            >
            /permissive-;
            /sdl;
            /W3;
	    /Zc:__cplusplus; /arch:AVX2
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    
    target_compile_definitions(${LIB_NAME14} PRIVATE
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "UNICODE;"
        "_UNICODE"
    )
	
     if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${LIB_NAME14} PRIVATE
            $<$<CONFIG:Release>:
                /std:c++14;
                /Oi;
                /Gy
            >
            /permissive-;
            /sdl;
            /W3;
	    /Zc:__cplusplus; /arch:AVX2
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    

endif()


if (UNIX)
	target_link_libraries(${LIB_NAME} PRIVATE  pthread)
	target_link_libraries(${LIB_NAME14} PRIVATE pthread)
	
elseif (MSVC)
	target_link_libraries(${LIB_NAME} PRIVATE )
	target_link_libraries(${LIB_NAME14} PRIVATE )
  
	
endif()
