# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.11)

# 프로젝트 정보
project(
  claujson
  VERSION 0.11.10 # 
  DESCRIPTION "claujson"
  LANGUAGES CXX)
  
set(CMP0077 on)

set(CMAKE_CXX_STANDARD 17) # C++17
set(CMAKE_BUILD_TYPE "Release")

set(PROGRAM_NAME claujson_test)
set(LIB_NAME claujson) # static lib

set(ROOT .)

set(LIB_DIR ${ROOT})

set(SRC_DIR ${ROOT})

set(INCLUDE_DIR ${ROOT})

# build setting.
add_library(${LIB_NAME} STATIC ${SRC_DIR}/claujson.cpp ${SRC_DIR}/simdjson.cpp) 
add_executable(${PROGRAM_NAME} ${SRC_DIR}/main.cpp)

include(FetchContent)
FetchContent_Declare(
  Fmt
  GIT_REPOSITORY "https://github.com/fmtlib/fmt"
  GIT_TAG "9.1.0"
  )
FetchContent_MakeAvailable(Fmt)

include(FetchContent)
FetchContent_Declare(
  Mimalloc
  GIT_REPOSITORY "https://github.com/microsoft/mimalloc"
  GIT_TAG "v2.0.9"
  )
FetchContent_MakeAvailable(Mimalloc)

set(gtest_force_shared_crt on)

include(FetchContent)
FetchContent_Declare(
  GoogleTest
  GIT_REPOSITORY "https://github.com/google/googletest"
  GIT_TAG "v1.13.0"
  )
FetchContent_MakeAvailable(GoogleTest)

# header..
target_include_directories(${PROGRAM_NAME} PRIVATE ${INCLUDE_DIR})
target_include_directories(${LIB_NAME} PUBLIC ${INCLUDE_DIR})

#google test
enable_testing()

add_executable(claujson_google_test ${SRC_DIR}/test.cpp)
#target_link_libraries(claujson_google_test gtest)
add_test(NAME claujson_google_test COMMAND claujson_google_test)



if (UNIX) 
	target_compile_options(${PROGRAM_NAME} PRIVATE -m64 -march=native -pthread  -Wno-narrowing  -O2)
  target_compile_options(claujson_google_test PRIVATE -m64 -march=native -pthread  -Wno-narrowing  -O2)
elseif (MSVC)

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
	target_compile_definitions(claujson_google_test PRIVATE
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
	
	if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(claujson_google_test PRIVATE
            $<$<CONFIG:Release>:
                /std:c++17;
                /Oi;
                /Gy
            >
            /permissive-;
            /sdl;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(claujson_google_test PRIVATE
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
	target_link_libraries(${LIB_NAME} PRIVATE fmt pthread)
	target_link_libraries(${PROGRAM_NAME} PRIVATE ${LIB_NAME} mimalloc pthread)
	target_link_libraries(claujson_google_test PRIVATE ${LIB_NAME} gtest mimalloc pthread)
elseif (MSVC)
	target_link_libraries(${LIB_NAME} PRIVATE fmt)
	target_link_libraries(${PROGRAM_NAME} PRIVATE ${LIB_NAME} mimalloc-static)
  target_link_libraries(claujson_google_test PRIVATE ${LIB_NAME} gtest mimalloc-static)
	
endif()
