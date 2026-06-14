// Copyright (c) 2025 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include "rkllm.h"

using namespace std;
LLMHandle llmHandle = nullptr;


void exit_handler(int signal) {
    if (llmHandle != nullptr) {
        {
            cout << "程序即将退出" << endl;
            LLMHandle _tmp = llmHandle;
            llmHandle      = nullptr;
            rkllm_destroy(_tmp);
        }
    }
    exit(signal);
}

int callback(RKLLMResult *result, void *userdata, LLMCallState state) {
    if (state == RKLLM_RUN_FINISH) {
        printf("\nrun finish\n");
    } else if (state == RKLLM_RUN_ERROR) {
        printf("\nrun error\n");
    } else if (state == RKLLM_RUN_NORMAL) {
        if (result != nullptr && result->text != nullptr) {
            printf("%s", result->text);
            fflush(stdout);
        }
    }
    return 0;
}

//
bool Init(const string &model_path) {
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path.c_str();

    // 设置采样参数
    param.top_k             = 1;
    param.top_p             = 0.95;
    param.temperature       = 0.8;
    param.repeat_penalty    = 1.1;
    param.frequency_penalty = 0.0;
    param.presence_penalty  = 0.0;

    param.max_new_tokens                 = 1024;
    param.max_context_len                = 4096;
    param.skip_special_token             = true;
    param.extend_param.base_domain_id    = 0;
    param.extend_param.embed_flash       = 1;

    param.is_async = false;
    param.extend_param.n_batch = 1;
    param.extend_param.enabled_cpus_num  = 4;
    param.extend_param.enabled_cpus_mask = CPU4 | CPU5 | CPU6 | CPU7;

    int ret = rkllm_init(&llmHandle, &param, callback);
    if (ret != 0) {
        std::cerr << "rkllm_init failed: " << ret << '\n';
        return false;
    }

    std::cout << "rkllm init success\n";
    return true;
}

bool run_once() {
    std::string prompt = " RK3588是哪家公司的产品？";

    RKLLMInput input{};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt.c_str();
    input.role = "user";
    input.enable_thinking = false;

    RKLLMInferParam infer_param{};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 0;

    int ret = rkllm_run(
        llmHandle,
        &input,
        &infer_param,
        nullptr
    );

    if (ret != 0) {
        std::cerr << "rkllm_run failed: " << ret << '\n';
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    signal(SIGINT, exit_handler);
    signal(SIGTERM, exit_handler);
    setlocale(LC_ALL, "en_US.UTF-8");

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " model_path" << std::endl;
        return 1;
    }

    printf("rkllm init start\n");
    if (!Init(argv[1])) {
        return 1;
    }
    bool success = run_once();
    if (llmHandle != nullptr) {
        rkllm_destroy(llmHandle);
        llmHandle = nullptr;
    }
    return success ? 0 : 1;

}
