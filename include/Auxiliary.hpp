#pragma once

#include <string>
#include <vector>

inline bool
contains(std::string const & s, std::string const &infix)
{
    return s.find(infix) != std::string::npos;
}

inline bool
ends_with(std::string const &s, std::string const &suffix)
{
    if( s.size() >= suffix.size() )
        return (0 == s.compare(s.size() - suffix.size(), suffix.size(), suffix));
    else
        return false;
}

inline std::string
replace(std::string s, std::string const & target, std::string const & replacement)
{
    std::string::size_type pos = s.find(target);
    if( pos == std::string::npos )
        return s;
    s.replace(pos, target.size(), replacement);

    return s;
}

inline std::vector< std::string >
split(std::string const &s, std::string const &delimiter, bool includeDelimiter = true)
{
    std::vector< std::string > ret;
    size_t pos, lastPos = 0;
    while( (pos = s.find(delimiter, lastPos + 1)) != std::string::npos )
    {
        ret.push_back(s.substr(lastPos + 1, pos - lastPos - (includeDelimiter ? 0 : delimiter.size())));
        lastPos = pos;
    }

    return ret;
}
