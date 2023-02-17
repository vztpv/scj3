# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.11)

# 프로젝트 정보
project(
  claujson
  VERSION 0.11.3
  DESCRIPTION "claujson"
  LANGUAGES CXX)

project(claujson)

set(CMAKE_CXX_STANDARD 17) # C++17

set(PROGRAM_NAME claujson)

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
target_include_directories(${PROGRAM_NAME} PRIVATE ${INCLUDE_DIR} fmt mimalloc)


if (UNIX) 
	target_compile_options(${PROGRAM_NAME} PRIVATE -m64 -march=native -pthread  -Wno-narrowing  -O2)
elseif (MSVC)
	target_compile_definitions(${PROGRAM_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
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
endif()


if (UNIX)

	target_link_libraries(${PROGRAM_NAME} PRIVATE fmt mimalloc pthread)
elseif (MSVC)

	target_link_libraries(${PROGRAM_NAME} PRIVATE fmt mimalloc-static "$(CoreLibraryDependencies)")
endif()
