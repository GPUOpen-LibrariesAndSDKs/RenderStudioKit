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

#include <pxr/base/tf/pyNoticeWrapper.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/usd/usd/notice.h>

#include <Notice/Notice.h>

#ifdef HOUDINI_SUPPORT
#include <hboost/python/class.hpp>
#include <hboost/python/def.hpp>
using namespace hboost::python;
#else
#include <boost/noncopyable.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/tuple.hpp>
using namespace boost::python;
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{

/*TF_INSTANTIATE_NOTICE_WRAPPER(RenderStudioNotice::PrimitiveChanged, TfNotice)
TF_INSTANTIATE_NOTICE_WRAPPER(RenderStudioNotice::LiveHistoryStatus, TfNotice)
TF_INSTANTIATE_NOTICE_WRAPPER(RenderStudioNotice::OwnerChanged, TfNotice)
TF_INSTANTIATE_NOTICE_WRAPPER(RenderStudioNotice::WorkspaceState, TfNotice)
TF_INSTANTIATE_NOTICE_WRAPPER(RenderStudioNotice::FileUpdated, TfNotice)*/
// TF_INSTANTIATE_NOTICE_WRAPPER(RenderStudioNotice::WorkspaceConnectionChanged, TfNotice)

} // anonymous namespace

void wrapRenderStudioNotice() {
    /*scope s = class_<RenderStudioNotice>("Notice", no_init);

    TfPyNoticeWrapper<
        RenderStudioNotice::WorkspaceConnectionChanged, UsdNotice::StageNotice>::Wrap()
        .def("IsConnected", &RenderStudioNotice::WorkspaceConnectionChanged::IsConnected)
        ;*/
};
