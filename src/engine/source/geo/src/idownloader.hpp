#ifndef _GEO_IDOWNLOADER_HPP
#define _GEO_IDOWNLOADER_HPP

#include <string>

#include <base/error.hpp>

namespace geo
{
class IDownloader
{
public:
    virtual ~IDownloader() = default;

    virtual base::RespOrError<std::string> downloadHTTPS(const std::string& url) = 0;
    virtual std::string computeMD5(const std::string& data) = 0;
};
} // namespace geo

#endif // _GEO_IDOWNLOADER_HPP
