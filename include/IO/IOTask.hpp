#pragma once


#include <map>

#include "../Attribute.hpp"
#include "../Dataset.hpp"


class Writable;

enum class ArgumentDatatype : int
{
    STRING = 0,
    VEC_UINT64,
    PTR_VOID,
    SHARED_PTR_VOID,
    DATATYPE,
    ATT_RESOURCE,
    SHARED_PTR_EXTENT,
    SHARED_PTR_DATATYPE,
    SHARED_PTR_ATT_RESOURCE,
    SHARED_PTR_VEC_STRING,

    UNDEFINED
};
using Argument = Variadic< ArgumentDatatype,
                           std::string,
                           std::vector< uint64_t >,
                           void*,
                           std::shared_ptr< void >,
                           Datatype,
                           Attribute::resource,
                           std::shared_ptr< Extent >,
                           std::shared_ptr< Datatype >,
                           std::shared_ptr< Attribute::resource >,
                           std::shared_ptr< std::vector< std::string > > >;

enum class Operation
{
    CREATE_FILE,
    OPEN_FILE,
    DELETE_FILE,

    CREATE_PATH,
    OPEN_PATH,
    DELETE_PATH,
    LIST_PATHS,

    CREATE_DATASET,
    OPEN_DATASET,
    DELETE_DATASET,
    WRITE_DATASET,
    READ_DATASET,
    LIST_DATASETS,

    DELETE_ATT,
    WRITE_ATT,
    READ_ATT,
    LIST_ATTS
};  //Operation


template< Operation >
struct Parameter
{ };

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
struct Parameter< Operation::OPEN_PATH >
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

template<>
struct Parameter< Operation::CREATE_DATASET >
{
    std::string name;
    Extent extent;
    Datatype dtype;
};

template<>
struct Parameter< Operation::OPEN_DATASET >
{
    std::string name;
    std::shared_ptr< Datatype > dtype
            = std::make_shared< Datatype >();
    std::shared_ptr< Extent > extent
            = std::make_shared< Extent >();
};

template<>
struct Parameter< Operation::DELETE_DATASET >
{
    std::string name;
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
struct Parameter< Operation::READ_DATASET >
{
    Extent extent;
    Offset offset;
    Datatype dtype;
    void* data = nullptr;
};

template<>
struct Parameter< Operation::LIST_DATASETS >
{
    std::shared_ptr< std::vector< std::string > > datasets
            = std::make_shared< std::vector< std::string > >();
};

template<>
struct Parameter< Operation::DELETE_ATT >
{
    std::string name;
};

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
struct Parameter< Operation::LIST_ATTS >
{
    std::shared_ptr< std::vector< std::string > > attributes
            = std::make_shared< std::vector< std::string > >();
};


template< Operation o >
std::map< std::string, Argument >
structToMap(Parameter< o > const&);

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
