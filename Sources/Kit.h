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

// ========== File Syncing API ==========

/// @brief Starts file sharing for inside workspace directory.
void SharedWorkspaceConnect();

/// @brief Stops file sharing for inside workspace directory.
void SharedWorkspaceDisconnect();

// ========== SdfPath Utils API ==========

/// @brief Checks if provided path could be transformed into RenderStudio path
/// @param path Path to check
/// @return True if path convertible to RenderStudio path
bool IsUnresolvable(const std::string& path);

/// @brief Convert local filesystem path from local path into RenderStudio path
/// @param path Local filesystem path
/// @return RenderStudio filesystem path
std::string Unresolve(const std::string& path);

/// @brief Checks if provided path is render studio path
/// @param path Path to check
/// @return True if path is RenderStudio path
bool IsRenderStudioPath(const std::string& path);

/// @brief Set directory which would be synchronized for all connected users
/// @param path Directory to syncronize
void SetWorkspacePath(const std::string& path);

/// @brief Set remote URL for remote workspace server
/// @param url URL of remote workspace server
void SetWorkspaceUrl(const std::string& url);

/// @brief Get current directory which would be synchronized for all connected users
std::string GetWorkspacePath();

/// @brief Get current remote URL for remote workspace server
std::string GetWorkspaceUrl();

} // namespace RenderStudio::Kit
