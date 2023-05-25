#ifndef _DOT_PATH_HPP
#define _DOT_PATH_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include <utils/stringUtils.hpp>

/**
 * @brief A dot-separated path, used to navigate nested field structures.
 *
 */
class DotPath
{
private:
    std::string m_str;                ///< The string representation of the path
    std::vector<std::string> m_parts; ///< The parts of the path

    /**
     * @brief Parse the string representation of the path into its parts.
     *
     * @throws std::runtime_error if the path is empty or has empty parts
     */
    void parse()
    {
        m_parts.clear();
        size_t pos = 0;
        auto prev = pos;
        while ((pos = m_str.find('.', prev)) != std::string::npos)
        {
            auto part = m_str.substr(prev, pos - prev);
            if (part.empty())
            {
                throw std::runtime_error("DotPath cannot have empty parts");
            }

            m_parts.push_back(part);
            prev = pos + 1;
        }

        auto part = m_str.substr(prev);
        if (part.empty())
        {
            throw std::runtime_error("DotPath cannot have empty parts");
        }
        m_parts.push_back(part);
    }

    void copy(const DotPath& rhs)
    {
        m_str = rhs.m_str;
        m_parts = rhs.m_parts;
    }

    void move(DotPath&& rhs) noexcept
    {
        m_str = std::move(rhs.m_str);
        m_parts = std::move(rhs.m_parts);
    }

public:
    DotPath() = default;
    ~DotPath() = default;

    /**
     * @brief Construct a new Dot Path object
     *
     * @param str
     * @throws std::runtime_error if the path is empty or has empty parts
     */
    DotPath(const std::string& str)
        : m_str(str)
    {
        parse();
    }

    /**
     * @brief Construct a new Dot Path object
     *
     * @param str
     * @throws std::runtime_error if the path is empty or has empty parts
     */
    DotPath(const char str[])
        : m_str(str)
    {
        parse();
    }

    /**
     * @brief Construct a new Dot Path object
     *
     * @param begin
     * @param end
     * @throws std::runtime_error if the path is empty or has empty parts
     */
    DotPath(decltype(m_parts.cbegin()) begin, const decltype(m_parts.cend())& end)
    {
        m_str = "";
        for (auto it = begin; it != end; ++it)
        {
            m_str += *it;
            if (it != end - 1)
            {
                m_str += ".";
            }
        }
        parse();
    }

    /**
     * @brief Construct a new Dot Path object
     *
     * @param rhs
     */
    DotPath(const DotPath& rhs) { copy(rhs); }

    /**
     * @brief Construct a new Dot Path object
     *
     * @param rhs
     */
    DotPath(DotPath&& rhs) noexcept { move(std::move(rhs)); }

    /**
     * @brief Copy assignment operator
     *
     * @param rhs
     * @return DotPath&
     */
    DotPath& operator=(const DotPath& rhs)
    {
        copy(rhs);
        return *this;
    }

    /**
     * @brief Move assignment operator
     *
     * @param rhs
     * @return DotPath&
     */
    DotPath& operator=(DotPath&& rhs) noexcept
    {
        move(std::move(rhs));
        return *this;
    }

    /**
     * @brief Constant iterator to the beginning of the path parts
     *
     * @return auto
     */
    auto cbegin() const { return m_parts.cbegin(); }

    /**
     * @brief Constant iterator to the end of the path parts
     *
     * @return auto
     */
    auto cend() const { return m_parts.cend(); }

    friend bool operator==(const DotPath& lhs, const DotPath& rhs) { return lhs.m_str == rhs.m_str; }
    friend bool operator!=(const DotPath& lhs, const DotPath& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const DotPath& dp)
    {
        os << dp.m_str;
        return os;
    }

    /**
     * @brief Implicit conversion to std::string
     *
     * @return std::string
     */
    explicit operator std::string() const { return m_str; }

    /**
     * @brief Get the string representation of the path
     *
     * @return const std::string&
     */
    const std::string& str() const { return m_str; }

    /**
     * @brief Get the parts of the path
     *
     * @return const std::vector<std::string>&
     */
    const std::vector<std::string>& parts() const { return m_parts; }

    /**
     * @brief Transform pointer path string to dot path string
     *
     * @param jsonPath Pointer path string
     * @return DotPath string
     */
    static DotPath fromJsonPath(const std::string& jsonPath)
    {
        std::string path {};

        if(jsonPath[0] == '/')
        {
            path = jsonPath.substr(1);
        }
        else
        {
            path = jsonPath;
        }

        auto parts = base::utils::string::split(path, '/');

        return DotPath(parts.begin(), parts.end());
    }
};

#endif // _DOT_PATH_HPP
