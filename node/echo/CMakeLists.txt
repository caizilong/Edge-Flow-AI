cmake_minimum_required(VERSION 3.12)
project(echo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(STACKFLOW_LIBRARY
    "${CMAKE_SOURCE_DIR}/../../install/lib/libstackflow.a"
    CACHE FILEPATH "Path to the StackFlow static library")

find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ../../hybrid-comm/include
    ../../infra-controller/include
    ../../utils
)

file(GLOB SRCS 

    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    pthread
    ${STACKFLOW_LIBRARY}
    zmq
)
