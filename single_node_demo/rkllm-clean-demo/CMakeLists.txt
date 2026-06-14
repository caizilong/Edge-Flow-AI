cmake_minimum_required(VERSION 3.10)
project(rkllm_clean_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RKLLM_ROOT
    "/home/pi/Edge-LLM-Infra/node/llm/rknn-llm/rkllm-runtime/Linux/librkllm_api"
)

add_executable(rkllm_clean_demo
    src/main.cpp
)

target_include_directories(rkllm_clean_demo PRIVATE
    "${RKLLM_ROOT}/include"
)

target_link_libraries(rkllm_clean_demo PRIVATE
    "${RKLLM_ROOT}/aarch64/librkllmrt.so"
    pthread
    dl
)

set_target_properties(rkllm_clean_demo PROPERTIES
    BUILD_RPATH "${RKLLM_ROOT}/aarch64"
)