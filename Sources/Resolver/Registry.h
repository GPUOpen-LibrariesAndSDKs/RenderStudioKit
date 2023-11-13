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
#include <pxr/usd/sdf/layer.h>
#pragma warning(pop)

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioLayerRegistry
{
public:
    void AddLayer(SdfLayerHandle layer);
    void RemoveLayer(SdfLayerHandle layer);
    void RemoveExpiredLayers();
    void ForEachLayer(const std::function<void(SdfLayerHandle)>& fn);
    SdfLayerHandle GetByIdentifier(const std::string& identifier);

private:
    std::map<std::string, SdfLayerHandle> mCreatedLayers;
};

PXR_NAMESPACE_CLOSE_SCOPE
