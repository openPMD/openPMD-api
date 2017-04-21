#include "../../include/IO/IOTask.hpp"


IOTask::IOTask(std::shared_ptr<AbstractFilePosition> fp,
               Operation o,
               std::map< std::string, Attribute > parameter)
        : abstractFilePosition{fp},
          operation{o},
          parameter{parameter}
{

}
