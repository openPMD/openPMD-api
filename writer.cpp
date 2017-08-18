#include <iostream>

#include "include/Output.hpp"

using std::shared_ptr;


void
write()
{
    Output o("./working/directory/",
             "%4d_3D_simData",
             Output::IterationEncoding::fileBased,
             Format::HDF5,
             AccessType::CREAT);
}

double x_data_lr[2][5] = {{1,  3,  5,  7,  9},
                          {11, 13, 15, 17, 19}};
double y_data_lr[2][5] = {{21, 23, 25, 27, 29},
                          {31, 33, 35, 37, 39}};
double z_data_lr[2][5] = {{41, 43, 45, 47, 49},
                          {51, 53, 55, 57, 59}};

double x_data[2][10] = {{0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
                        {10, 11, 12, 13, 14, 15, 16, 17, 18, 19}};
double y_data[2][10] = {{20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
                        {30, 31, 32, 33, 34, 35, 36, 37, 38, 39}};
double z_data[2][10] = {{40, 41, 42, 43, 44, 45, 46, 47, 48, 49},
                        {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}};

void
write2()
{
    Output f("./working/directory/",
             "3D_simData",
             Output::IterationEncoding::groupBased,
             Format::HDF5,
             AccessType::CREAT);
    // all required openPMD attributes will be set to reasonable default values (all ones, all zeros, empty strings,...)
    // manually setting them enforces the openPMD standard
    f.setMeshesPath("custom_meshes_path");
    f.setParticlesPath("long_and_very_custom_particles_path");

    // while it is possible to add and remove attributes, it is discouraged
    // removing attributes required by the standard typically makes the file unusable for post-processing
    f.setComment("This is fine and actually encouraged by the standard");
    f.setAttribute("custom_attribute_name",
                   "This attribute is manually added and can contain about any datatype you would want");
    f.deleteAttribute("custom_attribute_name");

    // everything that is accessed with [] should be interpreted as permanent storage
    // the objects sunk into these locations are deep copies
    {
        // setting attributes can be chained in JS-like syntax for compact code
        f.iterations[1].setTime(42).setDt(1.0).setTimeUnitSI(1.39e-16);
        f.iterations[2].setTimeUnitSI(1.39e-16);
        f.iterations.erase(2);
        f.iterations[42];
    }

    // the wish to modify a sunk resource (rather than a copy) must be stated explicitly
    // alternatively, a copy may be created and later re-assigned to f.iterations[1]
    Iteration& cur_it = f.iterations[1];

    // the underlying concept for numeric data is the openPMD Record
    // https://github.com/openPMD/openPMD-standard/blob/upcoming-1.0.1/STANDARD.md#scalar-vector-and-tensor-records
    // Meshes are specialized records
    cur_it.meshes["generic_3D_field"].setGridUnitSI(4).setUnitDimension({{Record::UnitDimension::L, -3}});
    cur_it.meshes["generic_3D_field"]["y"].setUnitSI(4);

    {
        // as this is a copy, it does not modify the sunk resource and can be modified independently
        Mesh lowRez = cur_it.meshes["generic_3D_field"];
        lowRez.setGridSpacing({6, 1, 1}).setGridGlobalOffset({0, 600, 0});

        Mesh highRez = cur_it.meshes["generic_3D_field"];
        highRez.setGridSpacing({6, 0.5, 1}).setGridGlobalOffset({0, 1200, 0});

        cur_it.meshes.erase("generic_3D_field");
        cur_it.meshes["lowRez_3D_field"] = lowRez;
        cur_it.meshes["highRez_3D_field"] = highRez;
    }

    cur_it.particles["e"].setAttribute("NoteWorthyParticleProperty",
                                       "This particle was observed to be very particle-esque.");
    cur_it.particles["e"]["weighting"][RecordComponent::SCALAR].setUnitSI(1e-5);

    // this wires up the numeric data
    Mesh& lr = cur_it.meshes["lowRez_3D_field"];
    Dataset d = Dataset(x_data_lr[0][0], Extent{2, 5});
    lr["x"].resetDataset(d);
    lr["y"].resetDataset(d);
    lr["z"].resetDataset(d);
    for( unsigned long i = 0; i < 2; ++i )
    {
        Offset o = Offset{i, 0};
        Extent e = Extent{1, 5};
        double *ptrToStart = &x_data_lr[i][0];

        auto my_deleter = [](double *p){ /* do nothing */ };
        // indicate shared ownership during IO
        // if you want to manage the lifetime of your numeric data, specify a deleter
        std::shared_ptr< double > chunk = std::shared_ptr< double >(ptrToStart, my_deleter);
        // TODO only first chunk is written
        lr["x"].storeChunk(o, e, chunk);
        // operations between store and flush are permitted, but MUST NOT modify the pointed-to data
        f.flush();
        // after the flush completes successfully, exclusive access to the shared resource is returned to the caller

//        auto read = lr["x"].loadChunk< double >(o, e);
//        for( uint8_t i = 0; i < 2; ++i )
//            for( uint8_t j = 0; j < 5; ++j )
//                if( chunk.get()[i*5 + j] != read.get()[i*5 + j] )
//                    std::cerr << "IO is incorrect!" << std::endl;
    }
//
//
//    for( int i = 0; i < 2; ++i )
//    {
//        /*
//        highRez["x"].linkDataToDisk(x); // Until the later call to flush completes,
//        highRez["y"].linkDataToDisk(y); // the numeric data behind the Datesets
//        highRez["z"].linkDataToDisk(z); // must be present in memory.
//                                        //
//        */
//        f.flush();                      // Now it may be deleted.
//    }
}


int
main()
{
    write2();
    return 0;
}
