#include "../../include/IO/IOTask.hpp"


template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::WRITE_ATT > p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"attribute", Attribute(p.resource)});
    ret.insert({"name", Attribute(p.name)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::DELETE_ATT> p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"name", Attribute(p.name)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::CREATE_DATASET > p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"name", Attribute(p.name)});
    ret.insert({"extent", Attribute(p.extent)});
    //ret.insert({"dtype", Attribute(p.dtype)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::CREATE_FILE > p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"name", Attribute(p.name)});
    return ret;
}

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::CREATE_PATH > p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"path", Attribute(p.path)});
    return ret;
};

