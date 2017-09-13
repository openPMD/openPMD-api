#include <iostream>

#include "H5Cpp.h"

int main()
{
    try{
        H5::H5File f;
        f.openFile("test", H5F_ACC_RDONLY);
    } catch(H5::FileIException i)
    {
        std::cout << "A hdf5 file exception, as expected" << std::endl;
    } catch (std::exception e)
    {
        std::cout << e.what() << std::endl;
    }

}