/* Copyright (C) 2015-2021, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "stringUtils.hpp"

namespace utils::string {

std::vector<std::string> split(std::string_view str, char delimiter)
{
    std::vector<std::string> ret;
    while(true)
    {
        auto pos = str.find(delimiter);
        if(pos == str.npos)
        {
            break;
        }
        ret.emplace_back(str.substr(0, pos));
        str = str.substr(pos + 1);
    }

    if(!str.empty())
    {
        ret.emplace_back(str);
    }

    return ret;
}

} // namespace utils::string
