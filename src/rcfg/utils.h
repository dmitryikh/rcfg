#pragma once

namespace rcfg::utils
{
    template <typename CONT>
    std::string join(const CONT & c, const std::string sep)
    {
        std::string cSep;
        std::string result;
        for (const auto & token : c)
        {
            result += cSep + token;
            if (cSep.empty())
                cSep = sep;
        }
        return result;
    }
}