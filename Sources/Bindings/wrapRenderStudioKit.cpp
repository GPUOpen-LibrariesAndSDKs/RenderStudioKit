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

#include "../Kit.h"

#ifdef HOUDINI_SUPPORT
#include <hboost/python/class.hpp>
#include <hboost/python/def.hpp>
#include <hboost/python/enum.hpp>
using namespace hboost::python;
#else
#include <boost/noncopyable.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/enum.hpp>
using namespace boost::python;
#endif

void
wrapRenderStudioKit()
{
    using namespace RenderStudio::Kit;

    enum_<Role>("Role")
    .value("Server", Role::Server)
    .value("Client", Role::Client)
    ;

    def("LiveSessionConnect", &LiveSessionConnect, args("info"));
    def("LiveSessionUpdate", &LiveSessionUpdate);
    def("LiveSessionDisconnect", &LiveSessionDisconnect);

    def("SharedWorkspaceConnect", &SharedWorkspaceConnect, args("role"));
    def("SharedWorkspaceDisconnect", &SharedWorkspaceDisconnect);

    def("IsUnresolvable", &IsUnresolvable, args("path"));
    def("Unresolve", &Unresolve, args("path"));
    def("IsRenderStudioPath", &IsRenderStudioPath, args("path"));
    def("SetWorkspacePath", &SetWorkspacePath, args("path"));
    def("GetWorkspacePath", &GetWorkspacePath);
    def("GetDefaultWorkspacePath", &GetDefaultWorkspacePath);
    def("SetWorkspaceUrl", &SetWorkspaceUrl, args("url"));
    def("GetWorkspaceUrl", &GetWorkspaceUrl);

    class_<LiveSessionInfo>("LiveSessionInfo" /*, init<std::string, std::string, std::string, std::string>()*/)
        .add_property("liveUrl", &LiveSessionInfo::liveUrl)
        .add_property("storageUrl", &LiveSessionInfo::storageUrl)
        .add_property("channelId", &LiveSessionInfo::channelId)
        .add_property("userId", &LiveSessionInfo::userId);
};
