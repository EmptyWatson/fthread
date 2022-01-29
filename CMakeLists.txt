cmake_minimum_required (VERSION 3.0)
project (fthread)

add_compile_options(
"$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-std=c++14>"
)

add_compile_options(-g)
#add_compile_options(-O0)

add_compile_definitions(ELPP_THREAD_SAFE)
add_compile_definitions(ELPP_NO_DEFAULT_LOG_FILE)

include_directories(
   ./
   ext/easyloggingpp
   ext/cxxopts-3.0.0/include
   ext/StringUtils
   ext/replxx/include
)

add_subdirectory(
   ext/replxx/
)

add_executable(fthread 
   fthread.cpp
   tracer.cpp
   proc.cpp
   cli.cpp
   utils.cpp
   ext/easyloggingpp/easylogging++.cc
   )

target_link_libraries(fthread
                      stdc++
                      pthread
                      replxx
                      )

#add_subdirectory(
#   tests
#)
