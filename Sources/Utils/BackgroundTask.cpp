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

#include "BackgroundTask.h"

namespace RenderStudio::Utils
{

BackgroundTask::BackgroundTask(TaskFn task, std::uint32_t interval)
    : mTask(task)
    , mInterval(interval)
{
}

BackgroundTask::~BackgroundTask() { Stop(); }

void
BackgroundTask::Start()
{
    std::unique_lock<std::mutex> lock(mMutex);

    if (mRunning)
    {
        return;
    }

    mRunning = true;
    mThread = std::thread(&BackgroundTask::Run, this);
}

void
BackgroundTask::Stop()
{
    std::unique_lock<std::mutex> lock(mMutex);

    if (!mRunning)
    {
        return;
    }

    mRunning = false;
    lock.unlock();
    mCondition.notify_one();

    if (mThread.joinable())
    {
        mThread.join();
    }
}

void
BackgroundTask::Run()
{
    while (IsRunning())
    {
        mTask();

        std::unique_lock<std::mutex> lock(mMutex);
        mCondition.wait_for(lock, std::chrono::seconds(mInterval), [this] { return !mRunning; });
    }
}

bool
BackgroundTask::IsRunning() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mRunning;
}

} // namespace RenderStudio::Utils
