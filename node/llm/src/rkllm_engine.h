#pragma once

#include <mutex>
#include <string>
#include <functional>
#include "rkllm.h"

using RkllmStreamCallback = std::function<void(const std::string &text, bool finish)>;

class RkllmEngine
{
public:
    RkllmEngine() = default;
    ~RkllmEngine();

    RkllmEngine(const RkllmEngine &) = delete;
    RkllmEngine &operator=(const RkllmEngine &) = delete;

    bool init(const std::string &model_path);
    bool run(const std::string &prompt, std::string &output);
    bool run_stream(const std::string &prompt, const RkllmStreamCallback &callback);
    void shutdown();

private:
    LLMHandle handle_ = nullptr;
    std::mutex mutex_;
};
