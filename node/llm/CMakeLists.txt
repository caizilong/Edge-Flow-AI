cmake_minimum_required(VERSION 3.12)
project(rkllm_node)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(RKLLM_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/rknn-llm/rkllm-runtime/Linux/librkllm_api"
)
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
    src/rkllm_engine.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(rkllm_node PRIVATE
    "${RKLLM_ROOT}/include"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    pthread
    ${STACKFLOW_LIBRARY}
    zmq
    "${RKLLM_ROOT}/aarch64/librkllmrt.so"
)

set_target_properties(rkllm_node PROPERTIES
    BUILD_RPATH "${RKLLM_ROOT}/aarch64"
)
