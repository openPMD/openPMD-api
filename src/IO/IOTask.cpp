#include "../../include/IO/IOTask.hpp"


template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::WRITE_ATT > p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"attribute", Attribute(p.resource)});
    ret.insert({"name", Attribute(p.name)});
    return ret;
};

template<>
std::map< std::string, Attribute > structToMap(Parameter< Operation::CREATE_FILE > p)
{
    std::map< std::string, Attribute > ret;
    ret.insert({"openPMD", Attribute(p.openPMD)});
    ret.insert({"openPMDextension", Attribute(p.openPMDextension)});
    ret.insert({"basePath", Attribute(p.basePath)});
    ret.insert({"meshesPath", Attribute(p.meshesPath)});
    ret.insert({"particlesPath", Attribute(p.particlesPath)});
    ret.insert({"iterationFormat", Attribute(p.iterationFormat)});
    ret.insert({"name", Attribute(p.name)});
    return ret;
};

