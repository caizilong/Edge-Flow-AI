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
using namespace StackFlows;
using json = nlohmann::json;

int main_exit_flage = 0;
static void __sigint(int iSigNo)
{
    main_exit_flage = 1;
}

class echo_task
{
private:
    std::string work_id_;
public:
    echo_task(const std::string &work_id) : work_id_(work_id)
    {
    }

    nlohmann::json inference(const std::string &msg)
    {
      try
        {
            return nlohmann::json::parse(msg);
        }
        catch (...)
        {
            return msg;
        }
    }
    void start()
    {
    }

    void stop()
    {
    }

    ~echo_task()
    {
        stop();
    }
};

class echo_node : public StackFlow
{
private:
    int task_count_;
    std::unordered_map<int, std::shared_ptr<echo_task>> echo_tasks_;
public:
    // 注册 echo 这个 unit
    echo_node():StackFlow("echo"){
      task_count_ = 3;
    }

    int setup(const std::string &work_id,
              const std::string &object,
              const std::string &data) override
    {
        int work_id_num = sample_get_work_id_num(work_id);
        auto channel = get_channel(work_id);
        auto task = std::make_shared<echo_task>(work_id);
        channel->set_output(true);
        channel->set_stream(false);
        channel->subscriber_work_id("", std::bind(&echo_node::on_inference, this,
                                                  std::weak_ptr<echo_task>(task),
                                                  std::weak_ptr<llm_channel_obj>(channel), work_id,
                                                  std::placeholders::_1, std::placeholders::_2));
        echo_tasks_[work_id_num] = task;
        send("None", "None", LLM_NO_ERROR, work_id);
        return 0;
    }

    void on_inference(const std::weak_ptr<echo_task> task_weak, const std::weak_ptr<llm_channel_obj> channel_weak,
                      const std::string &work_id, const std::string &object, const std::string &data)
    {
        auto task = task_weak.lock();
        auto channel = channel_weak.lock();
        if (!(task && channel))
        {
            return;
        }

        channel->send("echo.result", task->inference(data), LLM_NO_ERROR, work_id);
    }

    int exit(const std::string &work_id, const std::string &object, const std::string &data) override
    {
        int work_id_num = sample_get_work_id_num(work_id);
        if (echo_tasks_.find(work_id_num) == echo_tasks_.end())
        {
            nlohmann::json error_body;
            error_body["code"] = -6;
            error_body["message"] = "Unit Does Not Exist";
            send("None", "None", error_body, work_id);
            return -1;
        }

        auto channel = get_channel(work_id_num);
        channel->stop_subscriber("");
        echo_tasks_[work_id_num]->stop();
        echo_tasks_.erase(work_id_num);
        send("None", "None", LLM_NO_ERROR, work_id);
        return 0;
    }
    
    void taskinfo(const std::string &work_id, const std::string &object, const std::string &data) override
    {
        nlohmann::json body = nlohmann::json::array();
        for (const auto &task : echo_tasks_)
        {
            body.push_back(sample_get_work_id(task.first, unit_name_));
        }
        send("echo.tasklist", body, LLM_NO_ERROR, work_id);
    }

    ~echo_node()
    {
        while (1)
        {
            auto iteam = echo_tasks_.begin();
            if (iteam == echo_tasks_.end())
            {
                break;
            }
            iteam->second->stop();
            get_channel(iteam->first)->stop_subscriber("");
            iteam->second.reset();
            echo_tasks_.erase(iteam->first);
        }
    }
};

int main(int argc, char *argv[])
{
    signal(SIGTERM, __sigint);
    signal(SIGINT, __sigint);
    mkdir("/tmp/echo", 0777);
    echo_node echo;
    std::cout << "echo node started: ipc:///tmp/rpc.echo" << std::endl;
    while (!main_exit_flage)
    {
        sleep(1);
    }
    return 0;
}