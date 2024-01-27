# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.11.0)

# 프로젝트 정보
project(
  claujson
  VERSION 0.18.0 #
  DESCRIPTION "claujson"
  LANGUAGES CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

#set(CMAKE_CXX_STANDARD 17) # C++17
set(CMAKE_BUILD_TYPE "Release")

set(PROGRAM_NAME14 claujson_test14) # C++14?
set(PROGRAM_NAME claujson_test) # C++17
set(PROGRAM_NAME2 claujson_test2) # c++20
set(LIB_NAME claujson) # static lib
set(LIB_NAME14 claujson14) # static lib with c++ 14

set(ROOT .)

set(LIB_DIR ${ROOT})

set(SRC_DIR ${ROOT})

set(INCLUDE_DIR ${ROOT}) # ${ROOT}/_deps/rapidjson-src/include)


# build setting.
add_library(${LIB_NAME} STATIC ${SRC_DIR}/claujson.cpp ${SRC_DIR}/_simdjson.cpp) 
add_library(${LIB_NAME14} STATIC ${SRC_DIR}/claujson.cpp ${SRC_DIR}/_simdjson.cpp) 
add_executable(${PROGRAM_NAME} ${SRC_DIR}/main.cpp)
add_executable(${PROGRAM_NAME2} ${SRC_DIR}/main.cpp)
add_executable(${PROGRAM_NAME14} ${SRC_DIR}/main.cpp)

include(FetchContent)
FetchContent_Declare(
  Mimalloc
  GIT_REPOSITORY "https://github.com/microsoft/mimalloc"
  GIT_TAG "v2.0.9"
  )
FetchContent_GetProperties(Mimalloc)
if(NOT Mimalloc_POPULATED)
	FetchContent_Populate(Mimalloc)
	set(gtest_build_test OFF CACHE BOOL "Enable gtest tests" FORCE)
	add_subdirectory(${mimalloc_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()


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
	add_subdirectory(${rapidjson_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

# header..
target_include_directories(${PROGRAM_NAME} PRIVATE ${INCLUDE_DIR} ${rapidjson_SOURCE_DIR}/include)
target_include_directories(${PROGRAM_NAME2} PRIVATE ${INCLUDE_DIR} ${rapidjson_SOURCE_DIR}/include)
target_include_directories(${PROGRAM_NAME14} PRIVATE ${INCLUDE_DIR} ${rapidjson_SOURCE_DIR}/include)
target_include_directories(${LIB_NAME} PRIVATE ${INCLUDE_DIR} ${rapidjson_SOURCE_DIR}/include)
target_include_directories(${LIB_NAME14} PRIVATE ${INCLUDE_DIR} ${rapidjson_SOURCE_DIR}/include)


if (UNIX) 
        target_compile_options(${LIB_NAME} PRIVATE -m64 -std=c++20 -march=native -pthread -Wno-narrowing -Wextra -O2)
        target_compile_options(${LIB_NAME14} PRIVATE -m64 -std=c++14 -march=native -pthread -Wno-narrowing -Wextra -O2)
	target_compile_options(${PROGRAM_NAME} PRIVATE -m64 -std=c++17 -march=native -pthread  -Wno-narrowing -Wextra -O2)
	target_compile_options(${PROGRAM_NAME2} PRIVATE -m64 -std=c++20 -march=native -pthread -Wno-narrowing -Wextra  -O2)
	target_compile_options(${PROGRAM_NAME14} PRIVATE -m64 -std=c++14 -march=native -pthread -Wno-narrowing -Wextra -O2)
      
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
	    /Zc:__cplusplus;
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
	    /Zc:__cplusplus;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    
  # 
	target_compile_definitions(${PROGRAM_NAME} PRIVATE
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
	
	if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROGRAM_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /std:c++17;
                /Oi;
                /Gy
            >
            /permissive-;
	    /Zc:__cplusplus ;
            /sdl;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROGRAM_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
            /DEBUG;
            /SUBSYSTEM:CONSOLE
        )
    endif()

    
	target_compile_definitions(${PROGRAM_NAME2} PRIVATE
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
	
	if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROGRAM_NAME2} PRIVATE
            $<$<CONFIG:Release>:
                /std:c++20;
                /Oi;
                /Gy
            >
            /permissive-;
	    /Zc:__cplusplus ;
            /sdl;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROGRAM_NAME2} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
            /DEBUG;
            /SUBSYSTEM:CONSOLE
        )
    endif()
    
    	target_compile_definitions(${PROGRAM_NAME14} PRIVATE
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
	
	if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROGRAM_NAME14} PRIVATE
            $<$<CONFIG:Release>:
                /std:c++14;
                /Oi;
                /Gy
            >
            /permissive-;
	    /Zc:__cplusplus ;
            /sdl;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROGRAM_NAME14} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
            /DEBUG;
            /SUBSYSTEM:CONSOLE
        )
    endif()
endif()


if (UNIX)
	target_link_libraries(${LIB_NAME} PRIVATE  pthread)
	target_link_libraries(${LIB_NAME14} PRIVATE pthread)
	target_link_libraries(${PROGRAM_NAME} PRIVATE ${LIB_NAME} mimalloc pthread)
	target_link_libraries(${PROGRAM_NAME2} PRIVATE ${LIB_NAME} mimalloc pthread)
	target_link_libraries(${PROGRAM_NAME14} PRIVATE ${LIB_NAME14} mimalloc pthread)
	
elseif (MSVC)
	target_link_libraries(${LIB_NAME} PRIVATE )
	target_link_libraries(${LIB_NAME14} PRIVATE )
	target_link_libraries(${PROGRAM_NAME} PRIVATE ${LIB_NAME} mimalloc-static)
	target_link_libraries(${PROGRAM_NAME2} PRIVATE ${LIB_NAME} mimalloc-static)
	target_link_libraries(${PROGRAM_NAME14} PRIVATE ${LIB_NAME14} mimalloc-static)
  
	
endif()
