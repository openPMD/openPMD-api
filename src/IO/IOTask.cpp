#include "openPMD/IO/IOTask.hpp"


namespace openPMD
{
template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::CREATE_FILE > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::OPEN_FILE > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::DELETE_FILE > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::CREATE_PATH > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"path", ParameterArgument(p.path)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::OPEN_PATH > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"path", ParameterArgument(p.path)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::DELETE_PATH > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"path", ParameterArgument(p.path)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::LIST_PATHS > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"paths", ParameterArgument(p.paths)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::CREATE_DATASET > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    ret.insert({"extent", ParameterArgument(p.extent)});
    ret.insert({"dtype", ParameterArgument(p.dtype)});
    ret.insert({"chunkSize", ParameterArgument(p.chunkSize)});
    ret.insert({"compression", ParameterArgument(p.transform)});
    ret.insert({"transform", ParameterArgument(p.transform)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::EXTEND_DATASET > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    ret.insert({"extent", ParameterArgument(p.extent)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::OPEN_DATASET > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    ret.insert({"dtype", ParameterArgument(p.dtype)});
    ret.insert({"extent", ParameterArgument(p.extent)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::DELETE_DATASET > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::WRITE_DATASET > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"extent", ParameterArgument(p.extent)});
    ret.insert({"offset", ParameterArgument(p.offset)});
    ret.insert({"dtype", ParameterArgument(p.dtype)});
    ret.insert({"data", ParameterArgument(p.data)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::READ_DATASET > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"extent", ParameterArgument(p.extent)});
    ret.insert({"offset", ParameterArgument(p.offset)});
    ret.insert({"dtype", ParameterArgument(p.dtype)});
    ret.insert({"data", ParameterArgument(p.data)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::LIST_DATASETS > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"datasets", ParameterArgument(p.datasets)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::DELETE_ATT > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::WRITE_ATT > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    ret.insert({"dtype", ParameterArgument(p.dtype)});
    ret.insert({"attribute", ParameterArgument(p.resource)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::READ_ATT > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"name", ParameterArgument(p.name)});
    ret.insert({"dtype", ParameterArgument(p.dtype)});
    ret.insert({"resource", ParameterArgument(p.resource)});
    return ret;
}

template<>
std::map< std::string, ParameterArgument > structToMap(Parameter< Operation::LIST_ATTS > const& p)
{
    std::map< std::string, ParameterArgument > ret;
    ret.insert({"attributes", ParameterArgument(p.attributes)});
    return ret;
}
} // openPMD

