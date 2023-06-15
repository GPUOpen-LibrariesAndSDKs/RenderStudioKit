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