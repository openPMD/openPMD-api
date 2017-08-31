#include <iostream>

#include "include/Output.hpp"


void
write()
{
    Output o("./working/directory/",
             "const_test",
             Output::IterationEncoding::fileBased,
             Format::HDF5,
             AccessType::CREAT);

    Mesh& m = o.iterations[1].meshes["mesh_name"];
    auto& scalar = m[RecordComponent::SCALAR];

    double *d;
    Dataset dset = Dataset(d, {1000, 1000, 1000});
    scalar.resetDataset(dset);

    std::shared_ptr< double > data(d);
    scalar.storeChunk({0, 0, 0}, {1000, 1000, 1000}, data);

    o.flush();
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
             "2D_simData",
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
                   std::string("This attribute is manually added and can contain about any datatype you would want"));
    f.deleteAttribute("custom_attribute_name");

    // everything that is accessed with [] should be interpreted as permanent storage
    // the objects sunk into these locations are deep copies
    {
        // setting attributes can be chained in JS-like syntax for compact code
        f.iterations[1].setTime(42).setDt(1.0).setTimeUnitSI(1.39e-16);
        f.iterations[2].setTimeUnitSI(1.39e-16);
        f.iterations.erase(2);
    }

    {
        // the wish to modify a sunk resource (rather than a copy) must be stated
        Iteration& reference = f.iterations[1];

        // alternatively, a copy may be created and later re-assigned to f.iterations[1]
        Iteration copy = f.iterations[1];
        /* copy.changeParameters(); */
        f.iterations[1] = copy;
    }

    Iteration& cur_it = f.iterations[1];

    // the underlying concept for numeric data is the openPMD Record
    // https://github.com/openPMD/openPMD-standard/blob/upcoming-1.0.1/STANDARD.md#scalar-vector-and-tensor-records
    // Meshes are specialized records
    cur_it.meshes["generic_2D_field"].setGridUnitSI(4).setUnitDimension({{Mesh::UnitDimension::L, -3}});

    {
        // as this is a copy, it does not modify the sunk resource and can be modified independently
        Mesh lowRez = cur_it.meshes["generic_2D_field"];
        lowRez.setGridSpacing({6, 1}).setGridGlobalOffset({0, 600});

        Mesh highRez = cur_it.meshes["generic_2D_field"];
        highRez.setGridSpacing({6, 0.5}).setGridGlobalOffset({0, 1200});

        cur_it.meshes.erase("generic_2D_field");
        cur_it.meshes["lowRez_2D_field"] = lowRez;
        cur_it.meshes["highRez_2D_field"] = highRez;
    }
    cur_it.meshes.erase("highRez_2D_field");

    {
        // particles handle very similar
        ParticleSpecies& electrons = cur_it.particles["electrons"];
        electrons.setAttribute("NoteWorthyParticleSpeciesProperty",
                               std::string("Observing this species was a blast."));
        electrons["weighting"][RecordComponent::SCALAR].setUnitSI(1e-5);
        electrons["momentum"]["x"];
        electrons["momentum"]["y"];
    }
    cur_it.particles.erase("electrons");

    Mesh& mesh = cur_it.meshes["lowRez_2D_field"];
    f.flush();
    mesh.setAxisLabels({"x", "y"});
    f.flush();

    // before storing record data, you must specify the dataset once per component
    // this describes the datatype and shape of data as it should be written to disk
    Dataset d = Dataset(Datatype::DOUBLE, Extent{2, 5});
    mesh["x"].resetDataset(d);
    // at any point in time you may decide to dump already created output to disk
    // note that this will make some operations impossible (e.g. renaming files)
    f.flush();

    // writing only parts of the final dataset at a time is supported
    // this shows how to write every row of a 2D dataset at a time
    double x_data_lr[2][5] = {{1,  3,  5,  7,  9},
                              {11, 13, 15, 17, 19}};
    for( int i = 0; i < 2; ++i )
    {
        // your data is assumed to reside behind a pointer
        // as a contiguous column-major array
        double* simulation_data = new double[5];
        for( int j = 0; j < 5; ++j )
            simulation_data[j] = x_data_lr[i][j];

        // indicate shared data ownership during IO with a smart pointer
        auto no_deleter = [](double *p){ /* if YOU want to manage the lifetime of your pointer, do nothing */ };
        auto my_deleter = [](double *p){ delete[] p; /* otherwise, let the API destroy your data when not needed any more */ };
        std::shared_ptr< double > chunk = std::shared_ptr< double >(simulation_data, no_deleter);

        Offset o = Offset{static_cast<unsigned long>(i), 0};
        Extent e = Extent{1, 5};
        mesh["x"].storeChunk(o, e, chunk);
        // operations between store and flush MUST NOT modify the pointed-to data
        f.flush();
        // after the flush completes successfully, exclusive access to the shared resource is returned to the caller

        delete[] simulation_data;
    }

    mesh["y"].resetDataset(d);
    mesh["y"].setUnitSI(4);
    double constant_value = 0.3183098861837907;
    // for datasets that only contain one unique value, openPMD offers constant records
    mesh["y"].makeConstant(constant_value);
    f.flush();
}


int
main()
{
    write2();
    return 0;
}
