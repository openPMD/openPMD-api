#pragma once

#include <string>
#include <sstream>
#include <vector>

inline bool
contains(std::string const & s, std::string const &infix)
{
    return s.find(infix) != std::string::npos;
}

inline bool
starts_with(std::string const &s, std::string const &prefix)
{
    if( s.size() >= prefix.size() )
        return (0 == s.compare(0, prefix.size(), prefix));
    else
        return false;
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
split(std::string const &s, std::string const &delimiter, bool includeDelimiter = false)
{
    std::vector< std::string > ret;
    std::string::size_type pos, lastPos = 0, length = s.size();
    while( lastPos < length + 1 )
    {
        pos = s.find_first_of(delimiter, lastPos);
        if( pos == std::string::npos )
        {
            pos = length;
            includeDelimiter = false;
        }

        if( pos != lastPos )
            ret.push_back(s.substr(lastPos, pos + (includeDelimiter ? delimiter.size() : 0) - lastPos));

        lastPos = pos + 1;
    }

    return ret;
}
