/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "StackFlow.h"
#include "channel.h"
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <atomic>


using namespace StackFlows;
using json = nlohmann::json;

int main_exit_flage = 0;
static void __sigint(int iSigNo)
{
    main_exit_flage = 1;
}

class asr_task
{
private:
    std::string work_id_;
    AsrEngine &engine_;
public:
    asr_task(const std::string &work_id, AsrEngine &engine) : work_id_(work_id), engine_(engine)
    {
    }
    bool inference(const std::string &data, nlohmann::json &result, std::string &error)
    {

    }

    bool inference_stream(const std::string &data, const task_stream_callback_t &callback, std::string &error)
    {

    }

    void start()
    {
    }

    void stop()
    {
    }

    ~asr_task()
    {
        stop();
    }

};

class asr_node : public StackFlow
{
private:
    std::unordered_map<int, std::shared_ptr<asr_task>> tasks_;
    AsrEngine &engine_;
public:
    asr_node(AsrEngine &engine):StackFlow("asr"), engine_(engine){
    }

    int setup(const std::string &work_id,
              const std::string &object,
              const std::string &data) override
    {
        if (!tasks_.empty()) {
            send("None", "None",
                R"({"code":-21,"message":"task full"})",
                work_id);
            return -1;
        }
        int work_id_num = sample_get_work_id_num(work_id);
        auto channel = get_channel(work_id);
        auto task = std::make_shared<rkllm_task>(work_id, engine_);
        channel->set_output(true);
        channel->set_stream(true);
        channel->subscriber_work_id("", std::bind(&rkllm_node::on_inference_stream, this,
                                                  std::weak_ptr<rkllm_task>(task),
                                                  std::weak_ptr<llm_channel_obj>(channel), work_id,
                                                  std::placeholders::_1, std::placeholders::_2));
        tasks_[work_id_num] = task;
        send("None", "None", LLM_NO_ERROR, work_id);
        return 0;
    }

    void on_inference(const std::weak_ptr<rkllm_task> task_weak, const std::weak_ptr<llm_channel_obj> channel_weak,
                      const std::string &work_id, const std::string &object, const std::string &data)
    {
        auto task = task_weak.lock();
        auto channel = channel_weak.lock();
        if (!(task && channel))
        {
            return;
        }
        nlohmann::json result;
        std::string error;

        if (!task->inference(data, result, error)) {
            channel->send("None", "None", error, work_id);
            return;
        }
        channel->send("rkllm.result", result, LLM_NO_ERROR, work_id);
    }

    void on_inference_stream(const std::weak_ptr<rkllm_task> task_weak, const std::weak_ptr<llm_channel_obj> channel_weak,
                      const std::string &work_id, const std::string &object, const std::string &data)
    {
        auto task = task_weak.lock();
        auto channel = channel_weak.lock();
        if (!(task && channel))
        {
            return;
        }
        int index = 0;
        std::string error;

        bool success = task->inference_stream(data, [&](const std::string &text, bool finish) {
            nlohmann::json body;
            body["index"] = index++;
            body["delta"] = finish ? "" : text;
            body["finish"] = finish;
            channel->send("asr.result.stream", body, LLM_NO_ERROR, work_id);
        }, error);
        if (!success) {
            channel->send("None", "None", error, work_id);
            return;
        }
    }

    int exit(const std::string &work_id, const std::string &object, const std::string &data) override
    {
        int work_id_num = sample_get_work_id_num(work_id);
        if (tasks_.find(work_id_num) == tasks_.end())
        {
            nlohmann::json error_body;
            error_body["code"] = -6;
            error_body["message"] = "Unit Does Not Exist";
            send("None", "None", error_body, work_id);
            return -1;
        }

        auto channel = get_channel(work_id_num);
        channel->stop_subscriber("");
        tasks_[work_id_num]->stop();
        tasks_.erase(work_id_num);
        send("None", "None", LLM_NO_ERROR, work_id);
        return 0;
    }
    
    void taskinfo(const std::string &work_id, const std::string &object, const std::string &data) override
    {
        nlohmann::json body = nlohmann::json::array();
        for (const auto &task : tasks_)
        {
            body.push_back(sample_get_work_id(task.first, unit_name_));
        }
        send("asr.tasklist", body, LLM_NO_ERROR, work_id);
    }

    ~rkllm_node()
    {
        while (1)
        {
            auto iteam = tasks_.begin();
            if (iteam == tasks_.end())
            {
                break;
            }
            iteam->second->stop();
            get_channel(iteam->first)->stop_subscriber("");
            iteam->second.reset();
            tasks_.erase(iteam->first);
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " model_path\n";
        return 1;
    }

    signal(SIGTERM, __sigint);
    signal(SIGINT, __sigint);
    mkdir("/tmp/asr", 0777);
    

    AsrEngine engine;
    if (!engine.init(argv[1])) {
        std::cerr << "Failed to initialize ASR\n";
        return 1;
    }

    asr_node node(engine);
    std::cout << "asr node started: ipc:///tmp/rpc.asr" << std::endl;
    while (!main_exit_flage)
    {
        sleep(1);
    }
    return 0;
}