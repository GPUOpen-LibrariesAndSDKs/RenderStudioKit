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
