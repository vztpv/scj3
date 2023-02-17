# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.11)

# 프로젝트 정보
project(
  claujson
  VERSION 0.11.2
  DESCRIPTION "claujson"
  LANGUAGES CXX)

project(claujson)

set(CMAKE_CXX_STANDARD 17) # C++17

set(PROGRAM_NAME claujson_test)

set(ROOT .)

set(LIB_DIR ${ROOT})

set(SRC_DIR ${ROOT})

set(INCLUDE_DIR ${ROOT})

set(SOURCE ${SRC_DIR}/main.cpp ${SRC_DIR}/claujson.cpp ${SRC_DIR}/simdjson.cpp)


# build setting.
add_executable(${PROGRAM_NAME} ${SOURCE})



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


# header..
target_include_directories(${PROGRAM_NAME} PUBLIC ${INCLUDE_DIR} fmt mimalloc)


if (UNIX) 
	target_compile_options(${PROGRAM_NAME} PUBLIC -m64 -march=native -pthread  -Wno-narrowing  -O2)
endif()
if (UNIX)
	target_link_libraries(${PROGRAM_NAME} PUBLIC fmt mimalloc pthread)
endif()
