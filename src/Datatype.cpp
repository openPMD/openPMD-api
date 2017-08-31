#include <iostream>

#include "../include/Datatype.hpp"

std::ostream&
operator<<(std::ostream& os, Datatype d)
{
    using DT = Datatype;
    switch( d )
    {
        case DT::CHAR:
            os << "CHAR";
            break;
        case DT::INT:
            os << "INT";
            break;
        case DT::FLOAT:
            os << "FLOAT";
            break;
        case DT::DOUBLE:
            os << "DOUBLE";
            break;
        case DT::UINT32:
            os << "UINT32";
            break;
        case DT::UINT64:
            os << "UINT64";
            break;
        case DT::STRING:
            os << "STRING";
            break;
        case DT::ARR_DBL_7:
            os << "ARR_DBL_7";
            break;
        case DT::VEC_INT:
            os << "VEC_INT";
            break;
        case DT::VEC_FLOAT:
            os << "VEC_FLOAT";
            break;
        case DT::VEC_DOUBLE:
            os << "VEC_DOUBLE";
            break;
        case DT::VEC_UINT64:
            os << "VEC_UINT64";
            break;
        case DT::VEC_STRING:
            os << "VEC_STRING";
            break;
        case DT::INT16:
            os << "INT16";
            break;
        case DT::INT32:
            os << "INT32";
            break;
        case DT::INT64:
            os << "INT64";
            break;
        case DT::UINT16:
            os << "UINT16";
            break;
        case DT::UCHAR:
            os << "UCHAR";
            break;
        case DT::BOOL:
            os << "BOOL";
            break;
        case DT::DATATYPE:
            os << "DATATYPE";
            break;
        case DT::UNDEFINED:
            os << "UNDEFINED";
            break;
    }

    return os;
}
