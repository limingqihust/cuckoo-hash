cmake_minimum_required(VERSION 3.16.3)
project(cuckoo)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
link_directories(lib)

set(SRC_DIR "src")
aux_source_directory(${SRC_DIR} SRC_LIST)

add_executable(cuckoo ${SRC_LIST})

add_compile_options(-j4)

target_compile_options(cuckoo PRIVATE -g)

target_link_libraries(cuckoo libSMHasherSupport.a libcityhash.a)


