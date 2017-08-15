#include "../../include/IO/IOTask.hpp"


template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::WRITE_ATT > const& p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"attribute", Attribute(p.resource)});
    ret.insert({"name", Attribute(p.name)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::DELETE_ATT> const& p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"name", Attribute(p.name)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::CREATE_DATASET > const& p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"name", Attribute(p.name)});
    ret.insert({"extent", Attribute(p.extent)});
    ret.insert({"dtype", Attribute(p.dtype)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::WRITE_DATASET > const& p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"extent", Attribute(p.extent)});
    ret.insert({"offset", Attribute(p.offset)});
    return ret;
};

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::CREATE_FILE > const& p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"name", Attribute(p.name)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::CREATE_PATH > const& p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"path", Attribute(p.path)});
    return ret;
};

