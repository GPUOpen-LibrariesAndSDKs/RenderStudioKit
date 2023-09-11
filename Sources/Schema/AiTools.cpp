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

#include "AiTools.h"

#include <pxr/base/arch/env.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <Logger/Logger.h>
#include <Networking/RestClient.h>
#include <boost/json/src.hpp>

namespace
{

template <class T>
void
Extract(boost::json::object const& json, T& t, boost::json::string_view key)
{
    t = boost::json::value_to<T>(json.at(key));
}

template <class T>
void
Extract(boost::json::object const& json, std::optional<T>& t, boost::json::string_view key)
{
    if (json.if_contains(key))
    {
        t = boost::json::value_to<T>(json.at(key));
    }
}

const std::string SYSTEM_INPUT = R"x(
You are an assistant that only speaks JSON. Do not write normal text.
Use the following steps to respond to user inputs. Make sure you work out each step before move to the next.

Step 1 - You receive room dimensions (in centimeters) from user, followed by description in such format: (X, Y) <Room description>.
Step 2 - You generate an complete room layout, fill it with furniture that would be most likely placed in that room. Layout should use real furniture sizes and be accurate as possible.
Step 3 - You make sure that furniture objects are logically grouped, for example chairs should be near the table, bedroom tables should be near the bed, etc.
Step 4 - You make sure that furniture objects not overlap each other and not exceed room dimensions.
Step 5 - You make sure that room layout generated in the most meaningful way.
Step 6 - For each generated furniture object you pick a file that corresponds to that object from list (if no such object, then replace with plant or carpet):
- Armchair.glb
- BathroomSink.glb
- Bathtub.glb
- BedDouble.glb
- BedSingle.glb
- Bookshelf.glb
- Carpet.glb
- CoffeeTable.glb
- DiningChair.glb
- DiningTable.glb
- Fridge.glb
- KitchenCabinet.glb
- KitchenCabinetMicrowave.glb
- KitchenCabinetToaster.glb
- KitchenSink.glb
- KitchenStove.glb
- LampFloor.glb
- OfficeChair.glb
- OfficeDesk.glb
- PottedPlant.glb
- Shower.glb
- Sink.glb
- Sofa_1.glb
- Sofa_2.glb
- Toilet.glb
- TV.glb
- Wardrobe.glb
- Washer.glb
Step 7 - Imagine which objects from the list also may belong to the room and add it to result.
Step 8 - You generate json containing such fields:
- description: string with short conslusion what's been done. Be very brief and polite here. Add adjectives like cozy, modern, cool into description.
- furniture: Array of generated objects in such format:
-- name: Name of the object, they should be unique. Add undersore and index of element, if it already exists.
-- file: Associated file from list described in Step 6
-- position: (X, Y) coordinates where that object should be placed, pack them into array
-- rotation: float value in degrees, describing how object should be rotated. Use to face items correctly, because initially all models face the same directions. Do not rotate fridge, sink, microwave, toaster.
Step 9 - Make sure all rotations are correct, so furniture objects have correct alignment. For example TV should be rotated towards sofa, and chairs should be rotated towards the table.
Step 10 - Double check that positions are inside requested bounds, and objects belong to logic groups. Make sure rotations are correct and objects do not overlap each other. If not, adjust everything
Step 11 - Make sure if in real life some of the objects stay in one row, you also put them in single room, (for example fridge, stove and kitchen thumbs)
Step 12 - Add a bit of spacing between items, so they wouldn't overlap for sure
Step 12 - Send JSON reponse to the user
)x";

} // namespace

PXR_NAMESPACE_OPEN_SCOPE
using namespace boost::json;

pxr::GfVec2f
tag_invoke(const boost::json::value_to_tag<pxr::GfVec2f>&, const boost::json::value& json)
{
    boost::json::array jsonArray = json.as_array();
    return pxr::GfVec2f { boost::json::value_to<pxr::GfVec2f::ScalarType>(jsonArray.at(0)),
                          boost::json::value_to<pxr::GfVec2f::ScalarType>(jsonArray.at(1)) };
}

pxr::GfVec3f
tag_invoke(const boost::json::value_to_tag<pxr::GfVec3f>&, const boost::json::value& json)
{
    boost::json::array jsonArray = json.as_array();
    return pxr::GfVec3f { boost::json::value_to<pxr::GfVec3f::ScalarType>(jsonArray.at(0)),
                          boost::json::value_to<pxr::GfVec3f::ScalarType>(jsonArray.at(1)),
                          boost::json::value_to<pxr::GfVec3f::ScalarType>(jsonArray.at(2)) };
}

PXR_NAMESPACE_CLOSE_SCOPE

namespace RenderStudio::AI
{

AssistantPrimitiveInfo
tag_invoke(boost::json::value_to_tag<AssistantPrimitiveInfo>, boost::json::value const& json)
{
    AssistantPrimitiveInfo v;
    const boost::json::object& root = json.as_object();

    Extract(root, v.name, "name");
    Extract(root, v.file, "file");
    Extract(root, v.position, "position");
    Extract(root, v.rotation, "rotation");

    return v;
}

AssistantResponse
tag_invoke(boost::json::value_to_tag<AssistantResponse>, boost::json::value const& json)
{
    AssistantResponse v;
    const boost::json::object& root = json.as_object();

    Extract(root, v.description, "description");
    Extract(root, v.furniture, "furniture");

    return v;
}

std::optional<std::string>
Tools::ProcessPrompt(const std::string& prompt, pxr::UsdPrim& root)
{
    static const std::string ENV_OPENAI_KEY = "RENDER_STUDIO_OPENAI_KEY";

    if (!pxr::ArchHasEnv(ENV_OPENAI_KEY))
    {
        LOG_ERROR << "OpenAI key wasn't provided";
        return {};
    }

    const std::string apiKey = pxr::ArchGetEnv(ENV_OPENAI_KEY);

    AssistantResponse generated;

    try
    {
        boost::json::value request = {
            { "model", "gpt-4" },
            { "messages",
              boost::json::array { boost::json::object { { "role", "system" }, { "content", SYSTEM_INPUT } },
                                   boost::json::object { { "role", "user" }, { "content", "(300, 300) " + prompt } } } }
        };

        std::string response
            = RenderStudio::Networking::RestClient(
                  { { RenderStudio::Networking::RestClient::Parameters::ContentType, "application/json" },
                    { RenderStudio::Networking::RestClient::Parameters::Authorization, "Bearer " + apiKey } })
                  .Post("https://api.openai.com/v1/chat/completions", boost::json::serialize(request));

        boost::json::value json = boost::json::parse(response);
        boost::json::value content = boost::json::parse(
            json.as_object()["choices"].as_array()[0].as_object()["message"].as_object()["content"].as_string());
        generated = boost::json::value_to<AssistantResponse>(content);
        LOG_INFO << "[AI Tools] " << content;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR << "Tools::ProcessPrompt(): " << ex.what();
        return {};
    }

    for (const auto& entry : generated.furniture)
    {
        pxr::SdfPath path = root.GetPath().AppendChild(pxr::TfToken { pxr::TfMakeValidIdentifier(entry.name) });
        pxr::UsdPrim primitive = root.GetStage()->DefinePrim(path);
        primitive.GetReferences().ClearReferences();
        primitive.GetReferences().AddReference("studio://Assets/" + entry.file);
        pxr::GfVec3f position = { entry.position[0], 0, entry.position[1] };
        pxr::GfVec3f rotation = { 0, entry.rotation, 0 };
        pxr::UsdGeomXformCommonAPI(primitive).SetTranslate(position);
        pxr::UsdGeomXformCommonAPI(primitive).SetRotate(rotation);
    }

    pxr::UsdGeomXformCommonAPI(root).SetRotate({ 90, 0, 0 });
    pxr::UsdGeomXformCommonAPI(root).SetTranslate({ -150, 150, 10 });

    return generated.description;
}

} // namespace RenderStudio::AI
