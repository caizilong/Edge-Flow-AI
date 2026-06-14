#include <iostream>
#include <utility>

#include "rkllm_engine.h"

namespace
{
struct RunContext
{
    std::string output;
    bool failed = false;
};

// 它不再保存完整输出，而是保存“收到文本后调用什么函数”
struct RunStreamContext
{
    RkllmStreamCallback callback;
    bool failed = false;
};

// callback 中将result放入userdata指向的RunContext中，供run函数使用
int rkllm_callback(RKLLMResult *result, void *userdata, LLMCallState state)
{
    auto *context = static_cast<RunStreamContext *>(userdata);
    if (context == nullptr) {
        return 0;
    }

    if (state == RKLLM_RUN_NORMAL) {
        if (result != nullptr && result->text != nullptr && context->callback) {
            context->callback(result->text, false);
        }
    } else if (state == RKLLM_RUN_FINISH) {
        if (context->callback) {
            context->callback("", true);
        }
    } else if (state == RKLLM_RUN_ERROR) {
        context->failed = true;
    }

    return 0;
}
}  // namespace

bool RkllmEngine::init(const std::string &model_path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (handle_ != nullptr) {
        return true;
    }
    if (model_path.empty()) {
        std::cerr << "RKLLM model path is empty\n";
        return false;
    }

    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path.c_str();

    param.top_k = 1;
    param.top_p = 0.95f;
    param.temperature = 0.8f;
    param.repeat_penalty = 1.1f;
    param.max_new_tokens = 1024;
    param.max_context_len = 4096;
    param.skip_special_token = true;

    param.is_async = false;
    param.extend_param.base_domain_id = 0;
    param.extend_param.embed_flash = 1;
    param.extend_param.n_batch = 1;
    param.extend_param.enabled_cpus_num = 4;
    param.extend_param.enabled_cpus_mask = CPU4 | CPU5 | CPU6 | CPU7;

    int ret = rkllm_init(&handle_, &param, rkllm_callback);
    if (ret != 0) {
        handle_ = nullptr;
        std::cerr << "rkllm_init failed: " << ret << '\n';
        return false;
    }

    return true;
}

bool RkllmEngine::run(const std::string &prompt, std::string &output)
{
    std::lock_guard<std::mutex> lock(mutex_);

    output.clear();
    if (handle_ == nullptr || prompt.empty()) {
        return false;
    }

    RunContext context;

    RKLLMInput input{};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt.c_str();
    input.role = "user";
    input.enable_thinking = false;

    RKLLMInferParam infer_param{};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 0;

    // rkllm_run内部每生成一段文本，调用一次 callback
    int ret = rkllm_run(handle_, &input, &infer_param, &context);

    if (ret != 0 || context.failed) {
        std::cerr << "rkllm_run failed: " << ret << '\n';
        return false;
    }

    output = std::move(context.output);
    return true;
}

bool RkllmEngine::run_stream(const std::string &prompt, const RkllmStreamCallback &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (handle_ == nullptr || prompt.empty() || callback == nullptr) {
        return false;
    }

    RunStreamContext context;
    context.callback = callback;

    RKLLMInput input{};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt.c_str();
    input.role = "user";
    input.enable_thinking = false;

    RKLLMInferParam infer_param{};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 0;

    // rkllm_run内部每生成一段文本，调用一次 callback
    int ret = rkllm_run(handle_, &input, &infer_param, &context);

    if (ret != 0 || context.failed) {
        std::cerr << "rkllm_run failed: " << ret << '\n';
        return false;
    }

    return true;
}

void RkllmEngine::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (handle_ != nullptr) {
        rkllm_destroy(handle_);
        handle_ = nullptr;
    }
}

RkllmEngine::~RkllmEngine()
{
    shutdown();
}
