{ version }:
final: prev:
let
  callPackage = final.callPackage;
in
{
  openpmd_api = callPackage ./openpmd_api {
    inherit version;
    python = prev.python3;
    hdf5 = final.hdf5-mpi;
    adios2 = prev.adios2;
    catch2 = final.catch2_3;
  };
}
