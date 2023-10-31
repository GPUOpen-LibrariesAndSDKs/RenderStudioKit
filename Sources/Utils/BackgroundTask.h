// Copyright 2023 Advanced Micro Devices, Inc
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

#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace RenderStudio::Utils
{

class BackgroundTask
{
public:
    using TaskFn = std::function<void()>;

    BackgroundTask(TaskFn task, std::uint32_t interval);
    ~BackgroundTask();

    void Start();
    void Stop();

private:
    void Run();
    bool IsRunning() const;

    TaskFn mTask;
    bool mRunning { false };
    mutable std::mutex mMutex;
    std::condition_variable mCondition;
    std::thread mThread;
    std::uint32_t mInterval { 0 };
};

} // namespace RenderStudio::Utils
