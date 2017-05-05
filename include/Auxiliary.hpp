#pragma once

#include <string>

bool ends_with(std::string const &s, std::string const &suffix)
{
    if( s.length() >= suffix.length() )
        return (0 == s.compare(s.length() - suffix.length(), suffix.length(), suffix));
    else
        return false;
}

std::string replace(std::string s, std::string target, std::string replacement)
{
    std::string::size_type pos = s.find(target);
    if( pos == std::string::npos )
        return s;
    s.replace(pos, replacement.size(), replacement);
    return s;
}
