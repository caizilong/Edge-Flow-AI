#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "sherpa-onnx/c-api/cxx-api.h"

class AsrEngine
{
public:
    ~AsrEngine();

    bool init(const std::string &model_dir);

    bool feed(
        const int16_t *samples,
        std::size_t sample_count,
        std::string &text,
        bool &endpoint);

    bool finish(std::string &text);
    void reset();
    void shutdown();

private:
    std::unique_ptr<sherpa_onnx::cxx::OnlineRecognizer> recognizer_;
    std::unique_ptr<sherpa_onnx::cxx::OnlineStream> stream_;
};
