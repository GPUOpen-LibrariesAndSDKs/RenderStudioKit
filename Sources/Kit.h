#pragma once
#include <string>

namespace RenderStudio::Kit
{

// ========== Live Sessions API ==========

struct LiveSessionInfo
{
    std::string liveUrl;
    std::string storageUrl;
    std::string channelId;
    std::string userId;
};

/// @brief Connects to remote live server.
/// @param info Structure containing neccessary URL's and ID's to establish live connection.
void LiveSessionConnect(const LiveSessionInfo& info);

/// @brief Must be called from USD thread. Pushes local updates and pulls remote updates.
bool LiveSessionUpdate();

/// @brief Disconnects from remote live server.
void LiveSessionDisconnect();

// ========== SdfPath Utils API ==========

/// @brief Checks if provided path could be transformed into RenderStudio path
/// @param path Path to check
/// @return True if path convertible to RenderStudio path 
bool IsUnresovableToRenderStudioPath(const std::string& path);

/// @brief Convert local filesystem path from local path into RenderStudio path
/// @param path Local filesystem path
/// @return RenderStudio filesystem path
std::string UnresolveToRenderStudioPath(const std::string& path);

/// @brief Checks if provided path is render studio path
/// @param path Path to check
/// @return True if path is RenderStudio path
bool IsRenderStudioPath(const std::string& path);

} // namespace RenderStudio::Kit
