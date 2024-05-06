#ifndef _GEO_ILOCATOR_HPP
#define _GEO_ILOCATOR_HPP

#include <string>

#include <dotPath.hpp>
#include <error.hpp>
#include <json/json.hpp>

namespace geo
{
class ILocator
{
public:
    virtual ~ILocator() = default;

    /**
     * @brief Get the string data at the given path.
     *
     * @param path The path to the data.
     * @return Either the data as a string or an error if the data could not be retrieved.
     * @throws std::runtime_error if the path is invalid or the data is not a string.
     */
    virtual base::RespOrError<std::string> getString(const std::string& ip, const DotPath& path) = 0;

    /**
     * @brief Get the Uint32 data at the given path.
     *
     * @param path The path to the data.
     * @return base::RespOrError<uint32_t> Either the data as a uint32_t or an error if the data could not be retrieved.
     */
    virtual base::RespOrError<uint32_t> getUint32(const std::string& ip, const DotPath& path) = 0;

    /**
     * @brief Get the Double data at the given path.
     *
     * @param path The path to the data.
     * @return base::RespOrError<double>  Either the data as a double or an error if the data could not be retrieved.
     */
    virtual base::RespOrError<double> getDouble(const std::string& ip, const DotPath& path) = 0;

    /**
     * @brief Get the data at the given path as a json object.
     *
     * @param path The path to the data.
     * @return base::RespOrError<json::Json>  Either the data as a json object or an error if the data could not be
     * retrieved.
     * @note this method not supported array or object type.
     */
    virtual base::RespOrError<json::Json> getAsJson(const std::string& ip, const DotPath& path) = 0;
};

} // namespace geo

#endif // _GEO_ILOCATOR_HPP
