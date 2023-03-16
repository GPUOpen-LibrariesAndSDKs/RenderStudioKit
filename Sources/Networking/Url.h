#pragma once

#include <string>

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

private:
    std::string mProtocol;
    std::string mHost;
    std::string mPort;
    std::string mTarget;
    bool mSsl;
};

}