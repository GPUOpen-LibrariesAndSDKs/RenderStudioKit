#pragma once

#pragma warning(push, 0)
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
    bool Ssl() const { return mSsl; }
    friend std::ostream& operator<<(std::ostream& os, const Url& url);

    static std::string Encode(const std::string& url);

private:
    std::string mProtocol;
    std::string mHost;
    std::string mPort;
    std::string mTarget;
    bool mSsl;
};

} // namespace RenderStudio::Networking