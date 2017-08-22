#include "../../include/IO/IOTask.hpp"


template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::WRITE_ATT > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"name", Argument(p.name)});
    ret.insert({"dtype", Argument(p.dtype)});
    ret.insert({"attribute", Argument(p.resource)});
    return ret;
}

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::READ_ATT > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"name", Argument(p.name)});
    ret.insert({"dtype", Argument(p.dtype)});
    ret.insert({"resource", Argument(p.resource)});
    return ret;
}

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::DELETE_ATT > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"name", Argument(p.name)});
    return ret;
}

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::LIST_ATTS > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"attributes", Argument(p.attributes)});
    return ret;
}

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::CREATE_DATASET > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"name", Argument(p.name)});
    ret.insert({"extent", Argument(p.extent)});
    ret.insert({"dtype", Argument(p.dtype)});
    return ret;
}

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::WRITE_DATASET > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"extent", Argument(p.extent)});
    ret.insert({"offset", Argument(p.offset)});
    ret.insert({"dtype", Argument(p.dtype)});
    ret.insert({"data", Argument(p.data)});
    return ret;
};

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::CREATE_FILE > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"name", Argument(p.name)});
    return ret;
}

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::OPEN_FILE > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"name", Argument(p.name)});
    return ret;
}

template<>
std::map< std::string, Argument > structToMap(Parameter< Operation::CREATE_PATH > const& p)
{
    std::map< std::string, Argument > ret;
    ret.insert({"path", Argument(p.path)});
    return ret;
};

