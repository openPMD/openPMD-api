#pragma once


#include <map>

#include "AbstractFilePosition.hpp"
#include "../Attribute.hpp"


enum class Operation
{
    CREATE_ATT,
    WRITE_ATT,
    READ_ATT,
    DELETE_ATT,

    CREATE_DATASET,
    WRITE_DATASET,
    READ_DATASET,
    DELTE_DATASET,

    CREATE_FILE,
    DELETE_FILE,

    CREATE_PATH,
    DELETE_PATH

};  //Operation

class IOTask
{
public:
    //TODO this has to recieve an optional dataset including patch information
    IOTask(std::shared_ptr<AbstractFilePosition>,
           Operation,
           std::map< std::string, Attribute > parameter);

    std::shared_ptr<AbstractFilePosition> abstractFilePosition;
    Operation const operation;
    std::map< std::string, Attribute > const parameter;
};  //IOTask
