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

#include <string>
#include <vector>

#include <pxr/usd/usd/stage.h>

#include <boost/json.hpp>

namespace RenderStudio::AI
{

struct AssistantPrimitiveInfo
{
    std::string name;
    pxr::GfVec2f position;
    float rotation;
    std::string file;
};

AssistantPrimitiveInfo tag_invoke(boost::json::value_to_tag<AssistantPrimitiveInfo>, boost::json::value const& json);

struct AssistantResponse
{
    std::string description;
    std::vector<AssistantPrimitiveInfo> furniture;
};

AssistantResponse tag_invoke(boost::json::value_to_tag<AssistantResponse>, boost::json::value const& json);

class Tools
{
public:
    static std::optional<std::string> ProcessPrompt(const std::string& prompt, pxr::UsdPrim& root);
};

} // namespace RenderStudio::AI
