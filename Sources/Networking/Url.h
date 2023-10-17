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

#pragma warning(push, 0)
#include <map>
#include <string>
#pragma warning(pop)

namespace RenderStudio::Networking
{

class Url
{
public:
    static Url Parse(const std::string& request);
    std::string Protocol() const { return mProtocol; }
    std::string Host() const { return mHost; }
    std::string Port() const { return mPort; }
    std::string Target() const { return mTarget; }
    std::string Path() const { return mPath; }
    const std::map<std::string, std::string>& Query() const { return mQuery; }
    bool Ssl() const { return mSsl; }
    friend std::ostream& operator<<(std::ostream& os, const Url& url);

    static std::string Encode(const std::string& url);
    static std::string Decode(const std::string& url);

private:
    std::string mProtocol;
    std::string mHost;
    std::string mPort;
    std::string mTarget;
    std::string mPath;
    std::map<std::string, std::string> mQuery;
    bool mSsl;
};

} // namespace RenderStudio::Networking
