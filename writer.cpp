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

    std::shared_ptr< double > data;
    Datatype dtype = determineDatatype(data);
    Dataset dset = Dataset(dtype, {1000, 1000, 1000});
    m[RecordComponent::SCALAR].resetDataset(dset);

    scalar.storeChunk({0, 0, 0}, {10, 10, 10}, data);

    o.flush();
}

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
        f.iterations[1].setTime(42.0).setDt(1.0).setTimeUnitSI(1.39e-16);
        f.iterations[2].setComment("This iteration will not appear in any output");
        f.iterations.erase(2);
    }

    {
        // the wish to modify a sunk resource (rather than a copy) must be stated
        Iteration& reference = f.iterations[1];
        reference.setComment("Modifications to a reference will always be visible in the output");

        // alternatively, a copy may be created and later re-assigned to f.iterations[1]
        Iteration copy = f.iterations[1];
        copy.setComment("Modifications to copies will only take effect after you reassign the copy");
        f.iterations[1] = copy;
    }
    f.iterations[1].deleteAttribute("comment");

    Iteration& cur_it = f.iterations[1];

    // the underlying concept for numeric data is the openPMD Record
    // https://github.com/openPMD/openPMD-standard/blob/upcoming-1.0.1/STANDARD.md#scalar-vector-and-tensor-records
    // Meshes are specialized records
    cur_it.meshes["generic_2D_field"].setGridUnitSI(4).setUnitDimension({{Mesh::UnitDimension::L, -3}});

    {
        // as this is a copy, it does not modify the sunk resource and can be modified independently
        Mesh lowRez = cur_it.meshes["generic_2D_field"];
        lowRez.setGridSpacing(std::vector< double >{6, 1}).setGridGlobalOffset({0, 600});

        Mesh highRez = cur_it.meshes["generic_2D_field"];
        highRez.setGridSpacing(std::vector< double >{6, 0.5}).setGridGlobalOffset({0, 1200});

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

    // data is assumed to reside behind a pointer as a contiguous column-major array
    // shared data ownership during IO is indicated with a smart pointer
    std::shared_ptr< double > partial_dataset(new double[5], [](double *p){ delete[] p; });

    // before storing record data, you must specify the dataset once per component
    // this describes the datatype and shape of data as it should be written to disk
    Datatype dtype = determineDatatype(partial_dataset);
    Dataset d = Dataset(dtype, Extent{2, 5});
    mesh["x"].resetDataset(d);
    // at any point in time you may decide to dump already created output to disk
    // note that this will make some operations impossible (e.g. renaming files)
    f.flush();

    // chunked writing of the final dataset at a time is supported
    // this loop writes one row at a time
    double complete_dataset[2][5] = {{1,  3,  5,  7,  9},
                                     {11, 13, 15, 17, 19}};
    for( int i = 0; i < 2; ++i )
    {
        for( int col = 0; col < 5; ++col )
            partial_dataset.get()[col] = complete_dataset[i][col];

        Offset o = Offset{static_cast<unsigned long>(i), 0};
        Extent e = Extent{1, 5};
        mesh["x"].storeChunk(o, e, partial_dataset);
        // operations between store and flush MUST NOT modify the pointed-to data
        f.flush();
        // after the flush completes successfully, access to the shared resource is returned to the caller
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
