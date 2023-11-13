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

#include <filesystem>

namespace RenderStudio::Utils
{

class TempDirectory
{
public:
    TempDirectory();
    ~TempDirectory();
    std::filesystem::path Path() const;

private:
    std::filesystem::path mPath;
};

std::filesystem::path GetUserHomePath();
std::filesystem::path GetDefaultWorkspacePath();
std::filesystem::path GetRenderStudioPath();
std::filesystem::path GetCachePath();
void Extract(const std::filesystem::path& archive, const std::filesystem::path& destination);
void Move(const std::filesystem::path& source, const std::filesystem::path& destination);

} // namespace RenderStudio::Utils
