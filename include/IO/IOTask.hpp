#pragma once


#include <map>

#include "../Attribute.hpp"
#include "../Dataset.hpp"
//#include "../Writable.hpp"

class Writable;

enum class ArgumentDatatype : int
{
    STRING = 0,
    VEC_UINT64,
    SHARED_PTR_VOID,
    DATATYPE,
    ATT_RESOURCE,
    SHARED_PTR_DATATYPE,
    SHARED_PTR_ATT_RESOURCE,
    SHARED_PTR_VEC_STRING,

    UNDEFINED
};
using Argument = Variadic< ArgumentDatatype,
                           std::string,
                           std::vector< uint64_t >,
                           std::shared_ptr< void >,
                           Datatype,
                           Attribute::resource,
                           std::shared_ptr< Datatype >,
                           std::shared_ptr< Attribute::resource >,
                           std::shared_ptr< std::vector< std::string > > >;

enum class Operation
{
    WRITE_ATT,
    READ_ATT,
    DELETE_ATT,
    LIST_ATTS,

    CREATE_DATASET,
    WRITE_DATASET,
    READ_DATASET,
    DELETE_DATASET,

    CREATE_FILE,
    OPEN_FILE,
    DELETE_FILE,

    CREATE_PATH,
    DELETE_PATH,
    LIST_PATHS
};  //Operation


template< Operation >
struct Parameter
{ };

template<>
struct Parameter< Operation::WRITE_ATT >
{
    std::string name;
    Datatype dtype;
    Attribute::resource resource;
};

template<>
struct Parameter< Operation::READ_ATT >
{
    std::string name;
    std::shared_ptr< Datatype > dtype
            = std::make_shared< Datatype >();
    std::shared_ptr< Attribute::resource > resource
            = std::make_shared< Attribute::resource >();
};

template<>
struct Parameter< Operation::DELETE_ATT >
{
    std::string name;
};

template<>
struct Parameter< Operation::LIST_ATTS >
{
    std::shared_ptr< std::vector< std::string > > attributes
            = std::make_shared< std::vector< std::string > >();
};

template<>
struct Parameter< Operation::CREATE_DATASET >
{
    std::string name;
    Extent extent;
    Datatype dtype;
};

template<>
struct Parameter< Operation::WRITE_DATASET >
{
    Extent extent;
    Offset offset;
    Datatype dtype;
    std::shared_ptr< void > data;
};

template<>
struct Parameter< Operation::CREATE_FILE >
{
    std::string name;
};

template<>
struct Parameter< Operation::OPEN_FILE >
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

template<>
struct Parameter< Operation::LIST_PATHS >
{
    std::shared_ptr< std::vector< std::string > > paths
            = std::make_shared< std::vector< std::string > >();
};

template< Operation o >
std::map< std::string, Argument > structToMap(Parameter< o > const&);

class IOTask
{
public:
    template< Operation op >
    IOTask(Writable* w,
           Parameter< op > const& p)
            : writable{w},
              operation{op},
              parameter{structToMap(p)}
    { }

    Writable* writable;
    Operation const operation;
    std::map< std::string, Argument > parameter;
};  //IOTask
