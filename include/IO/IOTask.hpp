#pragma once


#include <map>

#include "../Attribute.hpp"
//#include "../Writable.hpp"

class Writable;


enum class Operation
{
    WRITE_ATT,
    READ_ATT,
    DELETE_ATT,

    CREATE_DATASET,
    WRITE_DATASET,
    READ_DATASET,
    DELETE_DATASET,

    CREATE_FILE,
    DELETE_FILE,

    CREATE_PATH,
    DELETE_PATH
};  //Operation


template< Operation >
struct Parameter
{ };

template<>
struct Parameter< Operation::WRITE_ATT >
{
    std::string name;
    Attribute::resource resource;
    Attribute::Dtype dtype;
};

template<>
struct Parameter< Operation::DELETE_ATT >
{
    std::string name;
};

template<>
struct Parameter< Operation::CREATE_DATASET >
{
    std::string name;
};

template<>
struct Parameter< Operation::CREATE_FILE >
{
    std::string name;
};

template<>
struct Parameter< Operation::DELETE_FILE >
{
    std::string name;
};

template<>
struct Parameter< Operation::CREATE_PATH >
{
    std::string path;
};

template<>
struct Parameter< Operation::DELETE_PATH >
{
    std::string path;
};

template< Operation o >
std::map< std::string, Attribute > structToMap(Parameter< o >);

class IOTask
{
public:
    //TODO this has to recieve an optional dataset including patch information
    template< Operation op >
    IOTask(Writable* w,
           Parameter< op > p)
            : writable{w},
              operation{op},
              parameter{structToMap(p)}
    { }

    Writable* writable;
    Operation const operation;
    std::map< std::string, Attribute > const parameter;
};  //IOTask
