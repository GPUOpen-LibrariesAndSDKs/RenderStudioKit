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

#include "Uuid.h"

#include <random>
#include <sstream>

namespace RenderStudio::Utils
{

// Thank you for implementation
// https://stackoverflow.com/questions/24365331/how-can-i-generate-uuid-in-c-without-using-boost-library

std::string
GenerateUUID()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    int i;
    ss << std::hex;

    for (i = 0; i < 8; i++)
    {
        ss << dis(gen);
    }

    ss << "-";

    for (i = 0; i < 4; i++)
    {
        ss << dis(gen);
    }

    ss << "-4";

    for (i = 0; i < 3; i++)
    {
        ss << dis(gen);
    }

    ss << "-";
    ss << dis2(gen);

    for (i = 0; i < 3; i++)
    {
        ss << dis(gen);
    }

    ss << "-";

    for (i = 0; i < 12; i++)
    {
        ss << dis(gen);
    };

    return ss.str();
}

} // namespace RenderStudio::Utils
