#include <openPMD/openPMD.hpp>

#include <algorithm>
#include <iostream>


using namespace openPMD;

void
write()
{
    Series o = Series("../samples/serial_write.h5", AccessType::CREATE);
    ParticleSpecies& e = o.iterations[1].particles["e"];

    std::vector< double > position_global(4);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local(new double);
    e["position"]["x"].resetDataset(Dataset(determineDatatype(position_local), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local = position_global[i];
        e["position"]["x"].storeChunk(position_local, {i}, {1});
        o.flush();
    }

    std::vector< uint64_t > positionOffset_global(4);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local(new uint64_t);
    e["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local = positionOffset_global[i];
        e["positionOffset"]["x"].storeChunk(positionOffset_local, {i}, {1});
        o.flush();
    }

    /* The files in 'o' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     */
}

void
write2()
{
    Series f = Series("working/directory/2D_simData.h5", AccessType::CREATE);

    // all required openPMD attributes will be set to reasonable default values (all ones, all zeros, empty strings,...)
    // manually setting them enforces the openPMD standard
    f.setMeshesPath("custom_meshes_path");
    f.setParticlesPath("long_and_very_custom_particles_path");

    // it is possible to add and remove attributes
    f.setComment("This is fine and actually encouraged by the standard");
    f.setAttribute(
        "custom_attribute_name",
        std::string("This attribute is manually added and can contain about any datatype you would want")
    );
    // note that removing attributes required by the standard typically makes the file unusable for post-processing
    f.deleteAttribute("custom_attribute_name");

    // everything that is accessed with [] should be interpreted as permanent storage
    // the objects sunk into these locations are deep copies
    {
        // setting attributes can be chained in JS-like syntax for compact code
        f.iterations[1]
                .setTime(42.0)
                .setDt(1.0)
                .setTimeUnitSI(1.39e-16);
        f.iterations[2].setComment("This iteration will not appear in any output");
        f.iterations.erase(2);
    }

    {
        // the wish to modify a sunk resource (rather than a copy) must be stated
        Iteration& reference = f.iterations[1];
        reference.setComment("Modifications to a reference will always be visible in the output");

        // alternatively, a copy may be created and later re-assigned to f.iterations[1]
        Iteration copy = f.iterations[1];  // TODO .copy()
        copy.setComment("Modifications to copies will only take effect after you reassign the copy");
        f.iterations[1] = copy;
    }
    f.iterations[1].deleteAttribute("comment");

    Iteration& cur_it = f.iterations[1];

    // the underlying concept for numeric data is the openPMD Record
    // https://github.com/openPMD/openPMD-standard/blob/upcoming-1.0.1/STANDARD.md#scalar-vector-and-tensor-records
    // Meshes are specialized records
    cur_it.meshes["generic_2D_field"].setUnitDimension({{UnitDimension::L, -3}, {UnitDimension::M, 1}});

    {
        // TODO outdated!
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
        // particles are handled very similar
        ParticleSpecies& electrons = cur_it.particles["electrons"];
        electrons.setAttribute("NoteWorthyParticleSpeciesProperty",
                               std::string("Observing this species was a blast."));
        electrons["displacement"].setUnitDimension({{UnitDimension::M, 1}});
        electrons["displacement"]["x"].setUnitSI(1e-6);
        electrons.erase("displacement");
        electrons["weighting"][RecordComponent::SCALAR].setUnitSI(1e-5);
    }

    Mesh& mesh = cur_it.meshes["lowRez_2D_field"];
    mesh.setAxisLabels({"x", "y"});

    // data is assumed to reside behind a pointer as a contiguous column-major array
    // shared data ownership during IO is indicated with a smart pointer
    std::shared_ptr< double > partial_mesh(new double[5], [](double *p){ delete[] p; p = nullptr; });

    // before storing record data, you must specify the dataset once per component
    // this describes the datatype and shape of data as it should be written to disk
    Datatype dtype = determineDatatype(partial_mesh);
    Dataset d = Dataset(dtype, Extent{2, 5});
    d.setCompression("zlib", 9);
    d.setCustomTransform("blosc:compressor=zlib,shuffle=bit,lvl=1;nometa");
    mesh["x"].resetDataset(d);

    ParticleSpecies& electrons = cur_it.particles["electrons"];

    Extent mpiDims{4};
    std::shared_ptr< float > partial_particlePos(new float[2], [](float *p){ delete[] p; p = nullptr; });
    dtype = determineDatatype(partial_particlePos);
    d = Dataset(dtype, mpiDims);
    electrons["position"]["x"].resetDataset(d);

    std::shared_ptr< uint64_t > partial_particleOff(new uint64_t[2], [](uint64_t *p){ delete[] p; p = nullptr; });
    dtype = determineDatatype(partial_particleOff);
    d = Dataset(dtype, mpiDims);
    electrons["positionOffset"]["x"].resetDataset(d);

    Dataset dset = Dataset(determineDatatype<uint64_t>(), {2});
    electrons.particlePatches["numParticles"][RecordComponent::SCALAR].resetDataset(dset);
    electrons.particlePatches["numParticlesOffset"][RecordComponent::SCALAR].resetDataset(dset);

    dset = Dataset(Datatype::FLOAT, {2});
    electrons.particlePatches["offset"].setUnitDimension({{UnitDimension::L, 1}});
    electrons.particlePatches["offset"]["x"].resetDataset(dset);
    electrons.particlePatches["extent"].setUnitDimension({{UnitDimension::L, 1}});
    electrons.particlePatches["extent"]["x"].resetDataset(dset);

    // at any point in time you may decide to dump already created output to disk
    // note that this will make some operations impossible (e.g. renaming files)
    f.flush();

    // chunked writing of the final dataset at a time is supported
    // this loop writes one row at a time
    double mesh_x[2][5] = {{1,  3,  5,  7,  9},
                           {11, 13, 15, 17, 19}};
    float particle_position[4] = {0.1, 0.2, 0.3, 0.4};
    uint64_t particle_positionOffset[4] = {0, 1, 2, 3};
    for( uint64_t i = 0; i < 2; ++i )
    {
        for( int col = 0; col < 5; ++col )
            partial_mesh.get()[col] = mesh_x[i][col];

        Offset o = Offset{i, 0};
        Extent e = Extent{1, 5};
        mesh["x"].storeChunk(partial_mesh, o, e);
        // operations between store and flush MUST NOT modify the pointed-to data
        f.flush();
        // after the flush completes successfully, access to the shared resource is returned to the caller

        for( int idx = 0; idx < 2; ++idx )
        {
            partial_particlePos.get()[idx] = particle_position[idx + 2*i];
            partial_particleOff.get()[idx] = particle_positionOffset[idx + 2*i];
        }

        uint64_t numParticlesOffset = 2*i;
        uint64_t numParticles = 2;

        o = Offset{numParticlesOffset};
        e = Extent{numParticles};
        electrons["position"]["x"].storeChunk(partial_particlePos, o, e);
        electrons["positionOffset"]["x"].storeChunk(partial_particleOff, o, e);

        electrons.particlePatches["numParticles"][RecordComponent::SCALAR].store(i, numParticles);
        electrons.particlePatches["numParticlesOffset"][RecordComponent::SCALAR].store(i, numParticlesOffset);

        electrons.particlePatches["offset"]["x"].store(i, particle_position[numParticlesOffset]);
        electrons.particlePatches["extent"]["x"].store(i, particle_position[numParticlesOffset + numParticles - 1] - particle_position[numParticlesOffset]);
    }

    mesh["y"].resetDataset(d);
    mesh["y"].setUnitSI(4);
    double constant_value = 0.3183098861837907;
    // for datasets that contain a single unique value, openPMD offers
    // constant records
    mesh["y"].makeConstant(constant_value);

    /* The files in 'f' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     */
}


void
w()
{
    Series o = Series("../samples/serial_write_%T.h5", AccessType::CREATE);

    /* The files in 'o' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     */
}

int
main()
{
    write2();
    return 0;
}
