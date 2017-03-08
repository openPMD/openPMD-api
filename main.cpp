#include <iostream>

#include "include/Output.hpp"
#include "include/writer/HDF5Writer.hpp"


// Presume this is your data in memory with a shape of 192x768x1
double* x_data, * y_data, * z_data;
// Presume this is your data in memory with a shape of 96x384x1
double* x_data_lr, * y_data_lr, * z_data_lr;

void
mesh()
{
    Output f(Output::IterationEncoding::fileBased);
    f.setName("001_3D_simData"); // _%04T_x_y_z
    f.setMeshesPath("custom_meshes_path");
    f.setParticlesPath("very_custom_particles_path");

    Iteration i;
    i.setTime(100).setDt(1.0).setTimeUnitSI(1.39e-16);
    f.iterations[100] = i;
    f.iterations[200] = Iteration(200, 1.0, 1.39e-16);
    f.iterations.remove(200);

    // most of the objects in this API are just proxies
    // the resources behind them do not manage their lifetime
    // the user has to explicitly state when he wants to start/end lifetime of a new object
    {
        using RD = Record::Dimension;
        Record scRec_3D_partDens = Record(RD::three, {"x", "y", "z"});
        scRec_3D_partDens["x"].setUnitSI(2.55999e-7);
        scRec_3D_partDens["y"].setUnitSI(4.42999e-8);
        scRec_3D_partDens["z"].setUnitSI(1);
        scRec_3D_partDens.setUnitDimension({{Record::UnitDimension::L, -3}});

        i.meshes["generic_3D_field"] = scRec_3D_partDens;
    }
    using DT = RecordComponent::Dtype;
    Mesh lowRez = i.meshes["generic_3D_field"];
    lowRez.setGridSpacing({6.23, 1.06, 1});
    lowRez.setGridGlobalOffset({0, 613.4, 0});
    lowRez.setGridUnitSI(4.1671151662e-8);
    lowRez.setPosition({{"y", {0.5, 0.5, 0.5}}});
    lowRez["x"].linkData(x_data_lr, DT::DOUBLE, {96, 384});
    lowRez["y"].linkData(y_data_lr, DT::DOUBLE, {96, 384});
    lowRez["z"].linkData(z_data_lr, DT::DOUBLE, {96, 384});

    Mesh highRez = i.meshes["generic_3D_field"];
    highRez.setGridSpacing({3.115, 0.53, 1});
    highRez.setGridGlobalOffset({0, 613.4, 0});
    highRez.setGridUnitSI(2.0835575831e-08);
    highRez.setPosition({{"y", {0.5, 0.5, 0.5}}});
    highRez["x"].linkData(x_data, DT::DOUBLE, {192, 768});
    highRez["x"].setUnitSI(1.279995e-07);
    highRez["y"].linkData(y_data, DT::DOUBLE, {192, 768});
    highRez["y"].setUnitSI(2.214995e-08);
    highRez["z"].linkData(z_data, DT::DOUBLE, {192, 768});

    i.meshes.remove("generic_3D_field");
    i.meshes["lowRez_3D_field"] = lowRez;
    i.meshes["highRez_3D_field"] = highRez;

    //f.write();
}

void particle()
{
    Output f(Output::IterationEncoding::fileBased);
    f.iterations[0] = Iteration();

//    f.iterations[0].particles["electrons"]["weighting"] = Record::makeScalarRecord();
}

int
main()
{
    mesh();
    return 0;
}
